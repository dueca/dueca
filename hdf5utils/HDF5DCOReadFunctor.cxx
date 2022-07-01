/* ------------------------------------------------------------------   */
/*      item            : HDF5DCOReadFunctor.cxx
        made by         : Rene' van Paassen
        date            : 170427
        category        : body file
        description     :
        changes         : 170427 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define HDF5DCOReadFunctor_cxx
#include "HDF5DCOReadFunctor.hxx"
#include "HDF5Exceptions.hxx"
#include <stringoptions.h>
#include "HdfLogNamespace.hxx"

#include <debug.h>

STARTHDF5LOG;


HDF5DCOReadFunctor::HDF5DCOReadFunctor(boost::weak_ptr<H5::H5File>& file,
                                       const std::string& path,
                                       size_t nelts, bool readticks) :
  file(file.lock()),
  readidx(0),
  advance(false),
  nrows(0),
  readticks(readticks),
  sets(readticks ? nelts+1 : nelts),
  basepath(path)
{
  // check that the base path is present
  try {

    // try an open on the base path
    file.lock()->openGroup(path + std::string("/data"));
  }
  catch(const H5::Exception& e) {
    /* DUECA hdf5.

       Requested data is not present at the given path. */
    E_XTR("Cannot find data in hdf path " << path
          << " error: " << e.getDetailMsg());
    throw(e);
  }
}

HDF5DCOReadFunctor::~HDF5DCOReadFunctor()
{
  //
}

void HDF5DCOReadFunctor::
configureDataSet(unsigned idx,
                 const std::string& name, hsize_t offset,
                 const H5::DataType* datatype, hsize_t ncols)
{
  try {
    H5::Exception::dontPrint();

    sets[idx].datatype = datatype;
    sets[idx].offset = offset;

    if (datatype == NULL) {
      // ignore this dataset!
      /* DUECA hdf5.

         Requested data type is not readable. */
      W_XTR("Cannot read dataset " << basepath << name);
      return;
    }

    // open the dataset
    sets[idx].dset = file.lock()->openDataSet(basepath + name);

    // get the associated data space
    sets[idx].filspace = sets[idx].dset.getSpace();

    // how large is it
    hsize_t dims[2];
    int rank = sets[idx].filspace.getSimpleExtentDims(dims);
    if (nrows == 0) nrows = dims[0];
    if (nrows != dims[0]) {
      /* DUECA hdf5.

         The number of configured rows does not match dataset
         dimension. */
      W_XTR("dataset " << basepath << name <<
            " unequal number of rows " << dims[0] << " vs " << nrows);
      throw(fileread_mismatch());
    }
    if (rank > 1 && ncols != dims[1]) {
      /* DUECA hdf5.

         The number of configured columns does not match dataset
         dimension. */
      W_XTR("dataset " << basepath << name <<
            " incorrect number of columns " << dims[1] << " vs " << ncols);
    }

    // remember # columns in this set
    sets[idx].row_dims[1] = ncols;

    // set the memory dataspace
    if (rank == 2) {
      sets[idx].memspace = H5::DataSpace(2, sets[idx].row_dims);
    }
    else if (rank == 1) {
      sets[idx].memspace = H5::DataSpace(H5S_SCALAR);
    }
    else {
      /* DUECA hdf5.

         This module is not configured for HDF5 files with more than
         2D data. */
      W_XTR("not configured for HDF5 files with > 2 dims");
      throw(fileread_mismatch());
    }
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to configure dataset " << name
              << ", got " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

HDF5DCOReadFunctor::LogDataSet::LogDataSet() :
  datatype(NULL), offset(0)
{
  offset_dims[0] = 0; offset_dims[1] = 0;
  row_dims[0] = 1; row_dims[1] = 1;
}

void HDF5DCOReadFunctor::LogDataSet::prepareRow(unsigned readidx)
{
  offset_dims[0] = readidx;
}

TimeTickType HDF5DCOReadFunctor::getTick(bool nextrow)
{
  // the previous getTick function called for advance; it got the
  // current or first data
  if (nextrow) {
    readidx++;
    if (readidx >= nrows) {
      throw(fileread_exhausted());
    }
    for (unsigned idx = sets.size(); idx--; ) {
      sets[idx].prepareRow(readidx);
    }
  }

  // read and return the tick, 0
  TimeTickType tick = 0;
  if (readticks) {
    sets[sets.size()-1].readObjectPart(&tick);
  }

  return tick;
}

void HDF5DCOReadFunctor::LogDataSet::readObjectPart(void* data,
                                                    const std::string& dum)
{
  union {char* ptr; std::string* val;} conv;
  conv.ptr = reinterpret_cast<char*>(data)+offset;
  try {
    H5::Exception::dontPrint();
    filspace.selectHyperslab(H5S_SELECT_SET, row_dims, offset_dims);
    dset.read(*conv.val, *datatype, memspace, filspace);
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to read a string "
              << ", got " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

void HDF5DCOReadFunctor::LogDataSet::readObjectPart(void* data)
{
  if (datatype == NULL) return;
  union {void* ptr; char* data;} conv; conv.ptr = data;
  try {
    H5::Exception::dontPrint();
    filspace.selectHyperslab(H5S_SELECT_SET, row_dims, offset_dims);
    dset.read(&conv.data[offset], *datatype, memspace, filspace);
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to write new data "
              << ", got " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

ENDHDF5LOG;
