

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  IFPACKSmoothers
  SOURCES cxx_main.cpp
  NUM_MPI_PROCS 1-4
  COMM serial mpi
  )
