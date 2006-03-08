#include "Galeri_ConfigDefs.h"
#ifdef HAVE_GALERI_HDF5
#include "Galeri_HDF5.h"
#include "Epetra_Import.h"
#include "Epetra_Export.h"
#include "Epetra_FECrsMatrix.h"


// ==========================================================================
// data container and iterators to find a dataset with a given name
struct FindDataset_t
{
  string name;
  bool found;
};

static herr_t FindDataset(hid_t loc_id, const char *name, void *opdata)
{
  string& token = ((FindDataset_t*)opdata)->name;
  if (token == name)
    ((FindDataset_t*)opdata)->found = true;

  return(0);
}

// ==========================================================================
// This function copied from Roman Geus' FEMAXX code
static void WriteParameterListRecursive(const Teuchos::ParameterList& params, 
                                        hid_t group_id) 
{
  Teuchos::ParameterList::ConstIterator it = params.begin();
  for (; it != params.end(); ++ it) {
    std::string key(params.name(it));
    if (params.isSublist(key)) {
      //
      // Sublist
      //

      // Create subgroup for sublist.
      hid_t child_group_id = H5Gcreate(group_id, key.c_str(), 0);
      WriteParameterListRecursive(params.sublist(key), child_group_id);
      H5Gclose(child_group_id);
    } else {
      //
      // Regular parameter
      //

      // Create dataspace/dataset.
      herr_t status;
      hsize_t one = 1;
      hid_t dataspace_id, dataset_id;

      // Write the dataset.
      if (params.isType<int>(key)) {
        int value = params.get<int>(key);
        dataspace_id = H5Screate_simple(1, &one, NULL);
        dataset_id = H5Dcreate(group_id, key.c_str(), H5T_NATIVE_INT, dataspace_id, H5P_DEFAULT);
        status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, 
                          H5P_DEFAULT, &value);
        status = H5Dclose(dataset_id);
        status = H5Sclose(dataspace_id);
      } else if (params.isType<double>(key)) {
        double value = params.get<double>(key);
        dataspace_id = H5Screate_simple(1, &one, NULL);
        dataset_id = H5Dcreate(group_id, key.c_str(), H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT);
        status = H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, 
                          H5P_DEFAULT, &value);
        status = H5Dclose(dataset_id);
        status = H5Sclose(dataspace_id);
      } else if (params.isType<std::string>(key)) {
        std::string value = params.get<std::string>(key);
        hsize_t len = value.size() + 1;
        dataspace_id = H5Screate_simple(1, &len, NULL);
        dataset_id = H5Dcreate(group_id, key.c_str(), H5T_C_S1, dataspace_id, H5P_DEFAULT);
        status = H5Dwrite(dataset_id, H5T_C_S1, H5S_ALL, H5S_ALL, 
                          H5P_DEFAULT, value.c_str());
        status = H5Dclose(dataset_id);
        status = H5Sclose(dataspace_id);
      } else if (params.isType<bool>(key)) {
        // Use H5T_NATIVE_USHORT to store a bool value
        unsigned short value = params.get<bool>(key) ? 1 : 0;
        dataspace_id = H5Screate_simple(1, &one, NULL);
        dataset_id = H5Dcreate(group_id, key.c_str(), H5T_NATIVE_USHORT, dataspace_id, H5P_DEFAULT);
        status = H5Dwrite(dataset_id, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, 
                          H5P_DEFAULT, &value);
        status = H5Dclose(dataset_id);
        status = H5Sclose(dataspace_id);
      } else {
        throw("Unable to export parameter \"%s\" to hdf5 file: Unsupported type.", key.c_str());        
      }
    }
  }
}

// ==========================================================================
// Recursive Operator function called by H5Giterate for each entity in group.
// This function copied from Roman Geus' FEMAXX code
static herr_t f_operator(hid_t loc_id, const char *name, void *opdata) 
{
  H5G_stat_t statbuf;
  hid_t dataset_id, space_id, type_id;
  Teuchos::ParameterList* sublist;
  Teuchos::ParameterList* params = (Teuchos::ParameterList*)opdata;
  /*
   * Get type of the object and display its name and type.
   * The name of the object is passed to this function by 
   * the Library. Some magic :-)
   */
  H5Gget_objinfo(loc_id, name, 0, &statbuf);
  switch (statbuf.type) {
  case H5G_GROUP:
    sublist = &params->sublist(name);
    H5Giterate(loc_id, name , NULL, f_operator, sublist);
    break;
  case H5G_DATASET:
    hsize_t len;
    dataset_id = H5Dopen(loc_id, name);
    space_id = H5Dget_space(dataset_id);
    if (H5Sget_simple_extent_ndims(space_id) != 1)
      throw("Dimensionality of parameters must be 1.");
    H5Sget_simple_extent_dims(space_id, &len, NULL);
    type_id = H5Dget_type(dataset_id);
    if (H5Tequal(type_id, H5T_NATIVE_DOUBLE) > 0) {
      double value;
      H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
      params->set(name, value);
    } else if (H5Tequal(type_id, H5T_NATIVE_INT) > 0) {
      int value;
      H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
      params->set(name, value);
    } else if (H5Tequal(type_id, H5T_C_S1) > 0) {
      char* buf = new char[len];
      H5Dread(dataset_id, H5T_C_S1, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
      params->set(name, std::string(buf));
      delete[] buf;
    } else if (H5Tequal(type_id, H5T_NATIVE_USHORT) > 0) {
      unsigned short value;
      H5Dread(dataset_id, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
      params->set(name, value != 0 ? true : false);
    } else {
      throw("Unsupported datatype"); // FIXME
    }
    H5Tclose(type_id);
    H5Sclose(space_id);  
    H5Dclose(dataset_id);  
    break;
  default:
    throw("Unsupported type"); // FIXME
  }
  return 0;
}
// ==========================================================================
void Galeri::HDF5::Create(const string FileName)
{
  FileName_ = FileName;

#ifdef HAVE_MPI
  // Set up file access property list with parallel I/O access
  plist_id = H5Pcreate(H5P_FILE_ACCESS);
  // Create property list for collective dataset write.

#if 0
  unsigned int boh = H5Z_FILTER_MAX;
  cout << "............." << H5Pset_filter(plist_id, H5Z_FILTER_DEFLATE, 
                                           H5Z_FILTER_MAX, 0, &boh);
#endif


  H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, MPI_INFO_NULL);
#else
  cerr << "Not yet implemented" << endl;
  exit(EXIT_FAILURE);
#endif
  // create the file collectively and release property list identifier.
  file_id = H5Fcreate(FileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, 
                      plist_id);
  H5Pclose(plist_id);

  plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
}

// ==========================================================================
void Galeri::HDF5::Open(const string FileName, int AccessType)
{
  FileName_ = FileName;

  // create the file collectively and release property list identifier.
  file_id = H5Fopen(FileName.c_str(), AccessType, H5P_DEFAULT);

  plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
}

// ==========================================================================
bool Galeri::HDF5::IsDataSet(string Name)
{
  FindDataset_t data;
  data.name = Name;
  data.found = false;

  int idx_f = H5Giterate(file_id, "/", NULL, FindDataset, (void*)&data);

  return(data.found);
}

// ==========================================================================
void Galeri::HDF5::Write(const string& Name, const int type, const int Length, 
                         void* data)
{
  cout << "INSIDE" << endl;
  hsize_t dimsf = Length;

  filespace = H5Screate_simple(1, &dimsf, NULL);
  if (!IsDataSet(Name))
    CreateGroup(Name.c_str());

  group_id = H5Gopen(file_id, Name.c_str());

  //dset_id = H5Dopen(file_id, Name.c_str());
  dset_id = H5Dcreate(group_id, "ARRAY", type, filespace, H5P_DEFAULT);

  status = H5Dwrite(dset_id, type, H5S_ALL, H5S_ALL,
                    H5P_DEFAULT, &data);

  // Close/release resources.
  H5Dclose(dset_id);
  H5Gclose(group_id);
  H5Sclose(filespace);
}

// ==========================================================================
// NEW ONE
void Galeri::HDF5::Write(const string& GroupName, const string& DataSetName,
                         int what)
{
  filespace = H5Screate(H5S_SCALAR);
  if (!IsDataSet(GroupName))
    CreateGroup(GroupName);

  group_id = H5Gopen(file_id, GroupName.c_str());
  dset_id = H5Dcreate(group_id, DataSetName.c_str(), H5T_NATIVE_INT, filespace,
                      H5P_DEFAULT);

  status = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL, filespace,
                    H5P_DEFAULT, &what);

  // Close/release resources.
  H5Dclose(dset_id);
  H5Gclose(group_id);
  H5Sclose(filespace);
}

// ==========================================================================
void Galeri::HDF5::Write(const string& GroupName, const string& DataSetName,
                         double what)
{
  filespace = H5Screate(H5S_SCALAR);
  if (!IsDataSet(GroupName))
    CreateGroup(GroupName);

  group_id = H5Gopen(file_id, GroupName.c_str());
  dset_id = H5Dcreate(group_id, DataSetName.c_str(), H5T_NATIVE_DOUBLE, filespace,
                      H5P_DEFAULT);

  status = H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, H5S_ALL, filespace,
                    H5P_DEFAULT, &what);

  // Close/release resources.
  H5Dclose(dset_id);
  H5Gclose(group_id);
  H5Sclose(filespace);
}

// ==========================================================================
void Galeri::HDF5::Write(const string& Name, int what)
{
  bool flag = IsDataSet(Name);

  filespace = H5Screate(H5S_SCALAR);
  if (flag)
  {
    dset_id = H5Dopen(file_id, Name.c_str());
  }
  else
    dset_id = H5Dcreate(file_id, Name.c_str(), H5T_NATIVE_INT, filespace,
                        H5P_DEFAULT);

  status = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL, filespace,
                    H5P_DEFAULT, &what);

  // Close/release resources.
  H5Dclose(dset_id);
  H5Sclose(filespace);
}

// ==========================================================================
void Galeri::HDF5::Write(const string& Name, double what)
{
  bool flag = IsDataSet(Name);

  filespace = H5Screate(H5S_SCALAR);
  if (flag)
  {
    dset_id = H5Dopen(file_id, Name.c_str());
  }
  else
    dset_id = H5Dcreate(file_id, Name.c_str(), H5T_NATIVE_DOUBLE, filespace,
                        H5P_DEFAULT);

  status = H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, H5S_ALL, filespace,
                    H5P_DEFAULT, &what);

  // Close/release resources.
  H5Dclose(dset_id);
  H5Sclose(filespace);
}

// ==========================================================================
void Galeri::HDF5::Write(const string& Name, const Epetra_Map& Map)
{
  int MySize = Map.NumMyElements();
  int GlobalSize = Map.NumGlobalElements();
  int* MyGlobalElements = Map.MyGlobalElements();

  Write(Name, "MyGlobalElements", MySize, GlobalSize, 
        H5T_NATIVE_INT, MyGlobalElements);
  Write(Name, "NumMyElements", 1, Comm_.NumProc(), H5T_NATIVE_INT, &MySize);
  Write(Name, "NumGlobalElements", 1, Comm_.NumProc(), H5T_NATIVE_INT, &GlobalSize);
  Write(Name, "NumProc", Comm_.NumProc());
  Write(Name, "IndexBase", Map.IndexBase());
  Write(Name, "Epetra_Map");
}

// ==========================================================================
void Galeri::HDF5::Write(const string& Name, const Epetra_Vector& x)
{
  const Epetra_BlockMap& OriginalMap = x.Map();
  Epetra_Map LinearMap(OriginalMap.NumGlobalElements(), OriginalMap.IndexBase(), Comm_);
  Epetra_Import Importer(LinearMap, OriginalMap);

  Epetra_Vector LinearVector(LinearMap);
  LinearVector.Import(x, Importer, Insert);

  Write(Name, "GlobalLength", x.GlobalLength());
  Write(Name, "Values", LinearMap.NumMyElements(), LinearMap.NumGlobalElements(),
        H5T_NATIVE_DOUBLE, x.Values());
  Write(Name, "Epetra_Vector");
}

// ==========================================================================
void Galeri::HDF5::Write(const string& Name, const Epetra_RowMatrix& Matrix)
{
  int MySize = Matrix.NumMyNonzeros();
  int GlobalSize = Matrix.NumGlobalNonzeros();
  vector<int> ROW(MySize);
  vector<int> COL(MySize);
  vector<double> VAL(MySize);

  int count = 0;
  int Length = Matrix.MaxNumEntries();
  vector<int> Indices(Length);
  vector<double> Values(Length);
  int NumEntries;

  for (int i = 0; i < Matrix.NumMyRows(); ++i)
  {
    Matrix.ExtractMyRowCopy(i, Length, NumEntries, &Values[0], &Indices[0]);
    for (int j = 0; j < NumEntries; ++j)
    {
      ROW[count] = Matrix.RowMatrixRowMap().GID(i);
      COL[count] = Matrix.RowMatrixColMap().GID(Indices[j]);
      VAL[count] = Values[j];
      ++count;
    }
  }

  Write(Name, "ROW", MySize, GlobalSize, H5T_NATIVE_INT, &ROW[0]);
  Write(Name, "COL", MySize, GlobalSize, H5T_NATIVE_INT, &COL[0]);
  Write(Name, "VAL", MySize, GlobalSize, H5T_NATIVE_DOUBLE, &VAL[0]);
  Write(Name, "NumGlobalRows", Matrix.NumGlobalRows());
  Write(Name, "NumGlobalCols", Matrix.NumGlobalCols());
  Write(Name, "NumGlobalNonzeros", Matrix.NumGlobalNonzeros());
  Write(Name, "NumGlobalDiagonals", Matrix.NumGlobalDiagonals());
  Write(Name, "NormOne", Matrix.NormOne());
  Write(Name, "NormInf", Matrix.NormInf());
  Write(Name, "Epetra_RowMatrix");
}

// ==========================================================================
void Galeri::HDF5::Write(const string& GroupName, const string& DataSetName,
                         int MySize, int GlobalSize, int type, const void* data)
{
  int Offset;
  Comm_.ScanSum(&MySize, &Offset, 1);
  Offset -= MySize;

  hsize_t MySize_t = MySize;
  hsize_t GlobalSize_t = GlobalSize;
  hsize_t Offset_t = Offset;

  filespace = H5Screate_simple(1, &GlobalSize_t, NULL); 

  // Create the dataset with default properties and close filespace.
  if (!IsDataSet(GroupName))
    CreateGroup(GroupName);

  group_id = H5Gopen(file_id, GroupName.c_str());
  dset_id = H5Dcreate(group_id, DataSetName.c_str(), type, filespace, H5P_DEFAULT);

  H5Sclose(filespace);

  // Each process defines dataset in memory and writes it to the hyperslab
  // in the file.

  memspace = H5Screate_simple(1, &MySize_t, NULL);

  // Select hyperslab in the file.
  filespace = H5Dget_space(dset_id);
  H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &Offset_t, NULL, &MySize_t, NULL);

  status = H5Dwrite(dset_id, type, memspace, filespace,
                    plist_id, data);

  // Close/release resources.
  H5Dclose(dset_id);
  H5Gclose(group_id);
  H5Sclose(filespace);
  H5Sclose(memspace);
}
// ==========================================================================
void Galeri::HDF5::Write(const string& Name, int MySize, int GlobalSize, int type, const void* data)
{
  int Offset;
  Comm_.ScanSum(&MySize, &Offset, 1);
  Offset -= MySize;

  hsize_t MySize_t = MySize;
  hsize_t GlobalSize_t = GlobalSize;
  hsize_t Offset_t = Offset;

  filespace = H5Screate_simple(1, &GlobalSize_t, NULL); 

  // Create the dataset with default properties and close filespace.
  if (!IsDataSet(Name))
    CreateGroup(Name);

  group_id = H5Gopen(file_id, Name.c_str());
  dset_id = H5Dcreate(group_id, Name.c_str(), type, filespace, H5P_DEFAULT);
    //dset_id = H5Dopen(file_id, Name.c_str());

  H5Sclose(filespace);

  // Each process defines dataset in memory and writes it to the hyperslab
  // in the file.

  memspace = H5Screate_simple(1, &MySize_t, NULL);

  // Select hyperslab in the file.
  filespace = H5Dget_space(dset_id);
  H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &Offset_t, NULL, &MySize_t, NULL);

  // Create property list for collective dataset write.

  status = H5Dwrite(dset_id, type, memspace, filespace,
                    plist_id, data);

  // Close/release resources.
  H5Dclose(dset_id);
  H5Gclose(group_id);
  H5Sclose(filespace);
  H5Sclose(memspace);
}

// ==========================================================================
void Galeri::HDF5::Write(const string& Name, const Teuchos::ParameterList& params)
{
  group_id = H5Gcreate(file_id, Name.c_str(), 0);

  // Iterate through parameter list 
  WriteParameterListRecursive(params, group_id);

  // Finalize hdf5 file
  status = H5Gclose(group_id);
}

// ==========================================================================
void Galeri::HDF5::Read(const string& GroupName, const string& DataSetName, int& data)
{
  if (!IsDataSet(GroupName))
    throw("not inserted");

  // Create group in the root group using absolute name.
  group_id = H5Gopen(file_id, GroupName.c_str());

  filespace = H5Screate(H5S_SCALAR);
  dset_id = H5Dopen(group_id, DataSetName.c_str());

  status = H5Dread(dset_id, H5T_NATIVE_INT, H5S_ALL, filespace,
                    H5P_DEFAULT, &data);

  H5Sclose(filespace);
  H5Dclose(dset_id);
  H5Gclose(group_id);
}

// ==========================================================================
void Galeri::HDF5::Read(const string& GroupName, const string& DataSetName, double& data)
{

}

// ==========================================================================
void Galeri::HDF5::Read(const string& Name, Teuchos::ParameterList& params) 
{
  // Open hdf5 file
  hid_t       group_id;  /* identifiers */
  herr_t      status;

  // Create group in the root group using absolute name.
  group_id = H5Gopen(file_id, Name.c_str());

  // Iterate through parameter list 
  string FullName = "/" + Name;
  status = H5Giterate(group_id, FullName.c_str() , NULL, f_operator, &params);

  // Finalize hdf5 file
  status = H5Gclose(group_id);
}

#if 0
// ==========================================================================
void Galeri::HDF5::Write(const string& Name, const Epetra_MultiVector& X)
{
  hid_t       group_id, dset_id;       /* file and dataset identifiers */
  hid_t       filespace, memspace;              /* file and memory dataspace identifiers */
  hid_t	    plist_id;                         /* property list identifier */
  herr_t      status;

  // Redistribute eigenvectors to force linear map
  Epetra_Map target_map(X.GlobalLength(), 0, Comm_);
  Epetra_MultiVector X0(target_map, X.NumVectors());
  X0.Export(X, Epetra_Export(X.Map(), target_map), Add);

  // Create the dataspace for the dataset.
  hsize_t q_dimsf[] = {X0.NumVectors(), X0.GlobalLength()};
  filespace = H5Screate_simple(2, q_dimsf, NULL);

  if (!IsDataSet(Name))
    CreateGroup(Name);

  group_id = H5Gopen(file_id, Name.c_str());

  // Create the dataset with default properties and close filespace.
  dset_id = H5Dcreate(group_id, "Values", H5T_NATIVE_DOUBLE, filespace, H5P_DEFAULT);

  // Create property list for collective dataset write.
  plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

  // Select hyperslab in the file.
  hsize_t offset[] = {0, X0.Map().GID(0)};
  hsize_t stride[] = {1, 1};
  hsize_t count[] = {X0.NumVectors(), 1};
  hsize_t block[] = {1, X0.MyLength()};
  filespace = H5Dget_space(dset_id);
  H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, stride, count, block);

  // Each process defines dataset in memory and writes it to the hyperslab in the file.
  hsize_t dimsm[] = {X0.NumVectors() * X0.MyLength()};
  memspace = H5Screate_simple(1, dimsm, NULL);

  // Write hyperslab
  status = H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, memspace, filespace, plist_id, X0.Values());
  H5Gclose(group_id);
  H5Sclose(memspace);
  H5Sclose(filespace);
  H5Dclose(dset_id);

  H5Pclose(plist_id);

  Write(Name, "Epetra_MultiVector");
  Write(Name, "NumVectors", X0.NumVectors());
}
#endif

// ==========================================================================
void Galeri::HDF5::Read(const string& GroupName, Epetra_Map*& Map)
{
  hid_t dataset_id, space_id;
  hsize_t num_eigenpairs, dims[2];
  herr_t status;

  if (!IsDataSet(GroupName))
    throw("group not found");

  string Label;
  Read(GroupName, Label);

  if (Label != "Epetra_Map")
  {
    cerr << "Error: attempting to read group that is not an Epetra_Map" << endl;
    throw(-1);
  }

  int NumMyElements, NumGlobalElements, IndexBase, NumProc;
  Read(GroupName, "NumMyElements", NumMyElements);
  Read(GroupName, "NumGlobalElements", NumGlobalElements);
  Read(GroupName, "IndexBase", IndexBase);
  Read(GroupName, "NumProc", NumProc);

  if (NumProc != Comm_.NumProc())
    throw("Reading map is not compatiable with current distribution");

  vector<int> MyGlobalElements(NumMyElements);

  Read(GroupName, "MyGlobalElements", NumMyElements, NumGlobalElements, 
       H5T_NATIVE_INT, &MyGlobalElements[0]);

  Map = new Epetra_Map(-1, NumMyElements, &MyGlobalElements[0], IndexBase, Comm_);
}

// ==========================================================================
void Galeri::HDF5::Read(const string& GroupName, Epetra_Vector*& X)
{
  hid_t dataset_id, space_id;
  hsize_t num_eigenpairs, dims[2];
  herr_t status;

  if (!IsDataSet(GroupName))
    throw("group not found");

  string Label;
  Read(GroupName, Label);

  if (Label != "Epetra_Vector")
  {
    cerr << "Error: attempting to read group that is not an Epetra_Vector" << endl;
    throw(-1);
  }

  int GlobalLength;
  Read(GroupName, "GlobalLength", GlobalLength);

  Epetra_Map LinearMap(GlobalLength, 0, Comm_);
  X = new Epetra_Vector(LinearMap);

  Read(GroupName, "Values", LinearMap.NumMyElements(), 
       LinearMap.NumGlobalElements(),
       H5T_NATIVE_DOUBLE, X->Values());
}

// ==========================================================================
void Galeri::HDF5::Read(const string& GroupName, const Epetra_Map& Map,
                        Epetra_Vector*& X)
{
  hid_t dataset_id, space_id;
  hsize_t num_eigenpairs, dims[2];
  herr_t status;

  if (!IsDataSet(GroupName))
    throw("group not found");

  string Label;
  Read(GroupName, Label);

  if (Label != "Epetra_Vector")
  {
    cerr << "Error: attempting to read group that is not an Epetra_Vector" << endl;
    throw(-1);
  }

  int GlobalLength;
  Read(GroupName, "GlobalLength", GlobalLength);

  Epetra_Map LinearMap(GlobalLength, 0, Comm_);
  Epetra_Vector LinearX(LinearMap);

  Read(GroupName, "Values", LinearMap.NumMyElements(), 
       LinearMap.NumGlobalElements(),
       H5T_NATIVE_DOUBLE, LinearX.Values());

  cout << LinearX;
  Epetra_Import Importer(Map, LinearMap);
  X = new Epetra_Vector(Map);
  X->Import(LinearX, Importer, Insert);
}

// ==========================================================================
void Galeri::HDF5::Read(const string& GroupName, const string& DataSetName,
                        int MySize, int GlobalSize,
                        const int type, void* data)
{
  cout << "MySize = " << MySize << endl;
  cout << "GlobalSize = " << GlobalSize << endl;

  // global size of the data to be read
  hsize_t MySize_t = MySize;
  hsize_t GlobalSize_t = GlobalSize;

  // offset
  int itmp;
  Comm_.ScanSum(&MySize, &itmp, 1);
  hsize_t Offset_t = itmp - MySize;

  cout << "offset = " << Offset_t << endl;

  group_id = H5Gopen(file_id, GroupName.c_str());
  hid_t dataset_id = H5Dopen(group_id, DataSetName.c_str());
  //hid_t space_id = H5Screate_simple(1, &Offset_t, 0);

  // Select hyperslab in the file.
  filespace = H5Dget_space(dataset_id);
  H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &Offset_t, NULL, 
                      &MySize_t, NULL);

  hid_t mem_dataspace = H5Screate_simple (1, &MySize_t, NULL);

  status = H5Dread(dataset_id, type, mem_dataspace, filespace, H5P_DEFAULT, 
                   data);

  H5Sclose(mem_dataspace);
  H5Gclose(group_id);  
  //H5Sclose(space_id);  
  H5Dclose(dataset_id);  
}

// ==========================================================================
void Galeri::HDF5::Read(const string& GroupName, Epetra_CrsMatrix*& A)
{
  hid_t dataset_id, space_id;
  hsize_t num_eigenpairs, dims[2];
  herr_t status;

  if (!IsDataSet(GroupName))
    throw("requested group not found in database");

  string Label;
  Read(GroupName, Label);

  if (Label != "Epetra_RowMatrix")
  {
    cerr << "Error: attempting to read group that is not an Epetra_RowMatrix" << endl;
    throw(-1);
  }

  int NumGlobalRows, NumGlobalCols;
  Read(GroupName, "NumGlobalRows", NumGlobalRows);
  Read(GroupName, "NumGlobalCols", NumGlobalCols);

  Epetra_Map RangeMap(NumGlobalRows, 0, Comm_);
  Epetra_Map DomainMap(NumGlobalCols, 0, Comm_);

  Read(GroupName, DomainMap, RangeMap, A);
}

// ==========================================================================
void Galeri::HDF5::Read(const string& GroupName, const Epetra_Map& DomainMap, 
                        const Epetra_Map& RangeMap, Epetra_CrsMatrix*& A)
{
  hid_t dataset_id, space_id;
  hsize_t num_eigenpairs, dims[2];
  herr_t status;

  if (!IsDataSet(GroupName))
    throw("requested group not found in database");

  string Label;
  Read(GroupName, Label);

  if (Label != "Epetra_RowMatrix")
  {
    cerr << "Error: attempting to read group that is not an Epetra_RowMatrix" << endl;
    throw(-1);
  }

  int NumGlobalRows, NumGlobalCols, NumGlobalNonzeros;
  Read(GroupName, "NumGlobalNonzeros", NumGlobalNonzeros);
  Read(GroupName, "NumGlobalRows", NumGlobalRows);
  Read(GroupName, "NumGlobalCols", NumGlobalCols);

  int NumMyNonzeros = NumGlobalNonzeros / Comm_.NumProc();
  if (Comm_.MyPID() == 0)
    NumMyNonzeros += NumGlobalNonzeros % Comm_.NumProc();

  cout << NumMyNonzeros << endl;

  vector<int> ROW(NumMyNonzeros);
  Read(GroupName, "ROW", NumMyNonzeros, NumGlobalNonzeros, H5T_NATIVE_INT, &ROW[0]);

  vector<int> COL(NumMyNonzeros);
  Read(GroupName, "COL", NumMyNonzeros, NumGlobalNonzeros, H5T_NATIVE_INT, &COL[0]);

  vector<double> VAL(NumMyNonzeros);
  Read(GroupName, "VAL", NumMyNonzeros, NumGlobalNonzeros, H5T_NATIVE_DOUBLE, &VAL[0]);

  Epetra_FECrsMatrix* A2 = new Epetra_FECrsMatrix(Copy, DomainMap, 0); // FIXME: RECT MATRICES

  for (int i = 0; i < NumMyNonzeros; ++i)
    A2->InsertGlobalValues(1, &ROW[i], 1, &COL[i], &VAL[i]);

  A2->FillComplete(DomainMap, RangeMap);

  A = A2;
}

#endif
