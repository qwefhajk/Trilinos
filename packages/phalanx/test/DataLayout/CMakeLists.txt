TRIBITS_INCLUDE_DIRECTORIES(REQUIRED_DURING_INSTALLATION_TESTING ${CMAKE_CURRENT_SOURCE_DIR})
TRIBITS_INCLUDE_DIRECTORIES(REQUIRED_DURING_INSTALLATION_TESTING ${CMAKE_CURRENT_SOURCE_DIR}/../Utilities)

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  mda_data_layout
  SOURCES MDALayout.cpp
  TESTONLYLIBS phalanx_unit_test_main
  NUM_MPI_PROCS 1
  )

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  dynamic_data_layout
  SOURCES DynamicLayout.cpp
  TESTONLYLIBS phalanx_unit_test_main
  NUM_MPI_PROCS 1
  )
