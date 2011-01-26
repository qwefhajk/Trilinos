/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#include <stk_util/parallel/Parallel.hpp>
#include <stk_util/parallel/ParallelReduce.hpp>
#include <stk_util/unit_test_support/stk_utest_macros.hpp>

#include <stk_mesh/base/Types.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/FieldData.hpp>
#include <stk_mesh/base/GetEntities.hpp>

#include <stk_mesh/fem/CreateAdjacentEntities.hpp>
#include <stk_mesh/fem/TopologyDimensions.hpp>
#include <stk_mesh/fem/TopologyHelpers.hpp>
#include <stk_mesh/fem/DefaultFEM.hpp>

#include <stk_rebalance/Rebalance.hpp>
#include <stk_rebalance/Partition.hpp>
#include <stk_rebalance/ZoltanPartition.hpp>

#include <stk_rebalance_utils/RebalanceUtils.hpp>

using stk::mesh::fem::NODE_RANK;

typedef stk::mesh::Field<double> ScalarField ;
typedef stk::mesh::Field<double, stk::mesh::Cartesian> VectorField ;

enum { nx = 2, ny = 2 };

STKUNIT_UNIT_TEST(UnitTestZoltanGraph, testUnit)
{
#ifdef STK_HAS_MPI
  stk::ParallelMachine comm(MPI_COMM_WORLD);
#else
  stk::ParallelMachine comm(0);
#endif

  unsigned spatial_dimension = 2;
  std::vector<std::string> rank_names = stk::mesh::fem::entity_rank_names(spatial_dimension);
  const stk::mesh::EntityRank constraint_rank = rank_names.size();
  rank_names.push_back("Constraint");

  stk::mesh::MetaData meta_data( rank_names );
  stk::mesh::BulkData bulk_data( meta_data , comm , 100 );
  stk::mesh::DefaultFEM top_data( meta_data, spatial_dimension );
  const stk::mesh::EntityRank element_rank    = stk::mesh::fem::element_rank(top_data);

  stk::mesh::Part & quad_part( stk::mesh::declare_part<shards::Quadrilateral<4> >( meta_data, "quad" ) );
  VectorField & coord_field( meta_data.declare_field< VectorField >( "coordinates" ) );

  stk::mesh::put_field( coord_field , NODE_RANK , meta_data.universal_part() );

  meta_data.commit();

  const unsigned p_size = bulk_data.parallel_size();
  const unsigned p_rank = bulk_data.parallel_rank();

  bulk_data.modification_begin();

  if ( p_rank == 0 ) {

    std::vector<std::vector<stk::mesh::Entity*> > quads(nx);
    for ( unsigned ix = 0 ; ix < nx ; ++ix ) quads[ix].resize(ny);

    const unsigned nnx = nx + 1 ;
    const unsigned nny = ny + 1 ;
    for ( unsigned iy = 0 ; iy < ny ; ++iy ) {
      for ( unsigned ix = 0 ; ix < nx ; ++ix ) {
        stk::mesh::EntityId elem = 1 + ix + iy * nx ;
        stk::mesh::EntityId nodes[4] ;
        nodes[0] = 1 + ix + iy * nnx ;
        nodes[1] = 2 + ix + iy * nnx ;
        nodes[2] = 2 + ix + ( iy + 1 ) * nnx ;
        nodes[3] = 1 + ix + ( iy + 1 ) * nnx ;

        stk::mesh::Entity &q = stk::mesh::declare_element( bulk_data , quad_part , elem , nodes );
        quads[ix][iy] = &q; 
      }
    }

    for ( unsigned iy = 0 ; iy <= ny ; ++iy ) {
      for ( unsigned ix = 0 ; ix <= nx ; ++ix ) {
        stk::mesh::EntityId nid = 1 + ix + iy * nnx ;
        stk::mesh::Entity * n = bulk_data.get_entity( NODE_RANK, nid );
        double * const coord = stk::mesh::field_data( coord_field , *n );
        coord[0] = .1*ix;
        coord[1] = .1*iy;
        coord[2] = 0;
      }
    }

    {
      const unsigned iy_left  =  0; 
      const unsigned iy_right = ny; 
      stk::mesh::PartVector add(1, &meta_data.locally_owned_part());
      for ( unsigned ix = 0 ; ix <= nx ; ++ix ) {
        stk::mesh::EntityId nid_left  = 1 + ix + iy_left  * nnx ;
        stk::mesh::EntityId nid_right = 1 + ix + iy_right * nnx ;
        stk::mesh::Entity * n_left  = bulk_data.get_entity( NODE_RANK, nid_left  );
        stk::mesh::Entity * n_right = bulk_data.get_entity( NODE_RANK, nid_right );
        const stk::mesh::EntityId constraint_entity_id =  1 + ix + nny * nnx;
        stk::mesh::Entity & c = bulk_data.declare_entity( constraint_rank, constraint_entity_id, add );
        bulk_data.declare_relation( c , *n_left  , 0 );
        bulk_data.declare_relation( c , *n_right , 1 );
      }
    }

  }

  // Only P0 has any nodes or elements
  if ( p_rank == 0 ) {
    STKUNIT_ASSERT( ! bulk_data.buckets( NODE_RANK ).empty() );
    STKUNIT_ASSERT( ! bulk_data.buckets( element_rank ).empty() );
  }
  else {
    STKUNIT_ASSERT( bulk_data.buckets( NODE_RANK ).empty() );
    STKUNIT_ASSERT( bulk_data.buckets( element_rank ).empty() );
  }


  bulk_data.modification_end();

  // create some sides and faces to rebalance.
  stk::mesh::PartVector add_parts;
  stk::mesh::create_adjacent_entities(bulk_data, add_parts);

  // Zoltan partition is specialized form a virtual base class, stk::rebalance::Partition.
  // Other specializations are possible.
  // Configure Zoltan to use graph-based partitioning
  Teuchos::ParameterList graph;
  Teuchos::ParameterList lb_method;
  lb_method.set("LOAD BALANCING METHOD"      , "4");
  graph.sublist(stk::rebalance::Zoltan::default_parameters_name())=lb_method;
  stk::rebalance::Zoltan zoltan_partition(comm, spatial_dimension, graph);
  // end configure snippet
  stk::mesh::Selector selector(meta_data.universal_part());

  // Force a rebalance by using imbalance_threshold < 1.0
  double imbalance_threshold = 0.5;
  bool do_rebal = stk::rebalance::rebalance_needed(bulk_data, NULL, imbalance_threshold,  stk::mesh::fem::node_rank(top_data), &selector);
  // Coordinates are passed to support geometric-based load balancing algorithms
  if( do_rebal )
    stk::rebalance::rebalance(bulk_data, selector, &coord_field, NULL, zoltan_partition, stk::mesh::fem::node_rank(top_data));

  imbalance_threshold = 1.5;
  do_rebal = stk::rebalance::rebalance_needed(bulk_data, NULL, imbalance_threshold,  stk::mesh::fem::node_rank(top_data), &selector);

  // Check that we satisfy our threshhold
  STKUNIT_ASSERT( !do_rebal );
  if( (2 == p_size) || (4 == p_size) )
  {
    STKUNIT_ASSERT_NEAR(imbalance_threshold, 1.0, .1);
  }
  else
  {
    STKUNIT_ASSERT_LE(imbalance_threshold, 1.5);
  }

  // And verify that all dependent entities are on the same proc as their parent element
  {
    stk::mesh::EntityVector entities;
    stk::mesh::Selector selector = meta_data.universal_part();

    get_selected_entities(selector, bulk_data.buckets(element_rank), entities);
    bool result = stk::rebalance::verify_dependent_ownership(NODE_RANK, entities, top_data);
    STKUNIT_ASSERT( result );
  }

}

/// \page stk_rebalance_unit_test_zoltan
///  \ingroup stk_rebalance_unit_test_module
/// \section stk_rebalance_unit_test_zoltan_graph Simple Zoltan Unit Test using Graph-Based Partitioning
///
/// This unit test is identical to 
/// \ref stk_rebalance_unit_test_zoltan_description "Simple Zoltan Unit Test"
/// with the only difference being use of graph-based partitioning instead
/// of element weights.
///
/// The distinction involves setting up an appropriate parameter list,
/// \dontinclude UnitTestZoltanGraph.cpp
/// \skip Configure Zoltan to use graph-based partitioning
/// \until end configure snippet
/// and then calling rebalance_needed and rebalance with NULL weight fields 
/// and calling rebalance with non-NULL coordinates, eg
/// \skip double imbalance_threshold = 
/// \until rebalance::rebalance(
/// 
///
/// The test passes if the resulting rebalance produces an imbalance measure
/// below 1.1 for 2 or 4 procs and below 1.5 otherwise.
///
///
/// See \ref UnitTestZoltanGraph.cpp for the complete source listing.
///
