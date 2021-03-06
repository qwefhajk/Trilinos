// $Id$
// $Source$

//@HEADER
// ************************************************************************
//
//            LOCA: Library of Continuation Algorithms Package
//                 Copyright (2005) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Roger Pawlowski (rppawlo@sandia.gov) or
// Eric Phipps (etphipp@sandia.gov), Sandia National Laboratories.
// ************************************************************************
//  CVS Information
//  $Source$
//  $Author$
//  $Date$
//  $Revision$
// ************************************************************************
//@HEADER

#ifndef LOCA_TURNINGPOINT_MINIMALLYAUGMENTED_ABSTRACTGROUP_H
#define LOCA_TURNINGPOINT_MINIMALLYAUGMENTED_ABSTRACTGROUP_H

#include "LOCA_TurningPoint_MooreSpence_AbstractGroup.H"    // base class

namespace LOCA {

  namespace TurningPoint {

    /*!
     * \brief Groups and vectors for locating turning point bifurcations
     * using the minimally augmented turning point formulation.
     */
    namespace MinimallyAugmented {

      /*!
       * \brief Interface to underlying groups for turning point calculations
       * using the minimally augmented formulation.
       */
      /*!
       * This abstract class provides the required interface for underlying
       * groups to locate turning points using the minimally augmented turning
       * point formulation (see
       * LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup for a
       * description of the governing equations).
       *
       * This class is derived from the
       * LOCA::TurningPoint::MooreSpence::AbstractGroup
       * and declares several pure virtual methods compute various
       * derivatives of \f$ w^TJn\f$ for a given \f$ w\f$ and \f$ n\f$.  Default
       * implementations for the derivatives using finite differencing are
       * implemented in the
       * LOCA::TurningPoint::MinimallyAugmented::FiniteDifferenceGroup.
       */
      class AbstractGroup :
    public virtual LOCA::TurningPoint::MooreSpence::AbstractGroup {

      public:

    //! Default constructor.
    AbstractGroup() {}

    //! Destructor
    virtual ~AbstractGroup() {}

    /*!
     * @name Pure virtual methods
     * These methods must be defined by any concrete implementation
     */
    //@{

    /*!
     * \brief Computes the derivative \f$\partial w^TJn/\partial p\f$
     */
    virtual NOX::Abstract::Group::ReturnType
    computeDwtJnDp(const std::vector<int>& paramIDs,
               const NOX::Abstract::Vector& w,
               const NOX::Abstract::Vector& nullVector,
               NOX::Abstract::MultiVector::DenseMatrix& result,
               bool isValid) = 0;

    /*!
     * \brief Computes the derivative \f$\partial w^TJ/\partial p\f$
     */
    virtual NOX::Abstract::Group::ReturnType
    computeDwtJDp(const std::vector<int>& paramIDs,
              const NOX::Abstract::Vector& w,
              NOX::Abstract::MultiVector& result,
              bool isValid) = 0;

    /*!
     * \brief Computes the derivative
     * \f$\frac{\partial w^TJn}{\partial x}\f$.
     */
    virtual NOX::Abstract::Group::ReturnType
    computeDwtJnDx(const NOX::Abstract::Vector& w,
               const NOX::Abstract::Vector& nullVector,
               NOX::Abstract::Vector& result) = 0;

    //@}

    //! Bring NOX::Abstract::Group::operator=() into scope
    using NOX::Abstract::Group::operator=;

      }; // class AbstractGroup

    } // namespace MinimallyAugmented

  } // namespace TurningPoint

} // namespace LOCA

#endif
