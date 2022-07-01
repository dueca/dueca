/* ------------------------------------------------------------------   */
/*      item            : HDF5DCOWriteFunctor.cxx
        made by         : Rene' van Paassen
        date            : 170327
        category        : body file
        description     :
        changes         : 170327 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define HDF5DCOWriteFunctor_cxx
#include "HDF5DCOWriteFunctor.hxx"
#include <stringoptions.h>
//#define I_XTR
#include <debug.h>
#include <algorithm>

STARTHDF5LOG;

HDF5DCOWriteFunctor::
HDF5DCOWriteFunctor(boost::weak_ptr<H5::H5File>& file,
                    const std::string& path, size_t chunksize,
                    const std::string& label,
                    size_t nelts, bool compress, bool writeticks,
                    const dueca::DataTimeSpec* startend) :
  file(file.lock()),
  startend(startend),
  writeticks(writeticks),
  compress(compress),
  chunksize(chunksize),
  chunkidx(0),
  sets(writeticks ? nelts+1 : nelts),
  basepath(path)
{
  H5::Group dpath = createPath(basepath);

  try {
    H5::Exception::dontPrint();

    if (label.size()) {
      // create a string attribute to the data path
      H5::DataSpace attr_dataspace = H5::DataSpace(H5S_SCALAR);
      H5::StrType strdatatype(H5::PredType::C_S1, label.size());
      H5::Attribute labeldata = dpath.createAttribute("label", strdatatype,
                                                    attr_dataspace);
      labeldata.write(strdatatype, label.c_str());
    }

    file.lock()->createGroup(basepath + std::string("/data"));
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to attach label & create /data for " << path
              << " error: " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

HDF5DCOWriteFunctor::~HDF5DCOWriteFunctor()
{
  // finalize the file size
  for (unsigned idx = sets.size(); idx--; ) {
    sets[idx].finalize(chunkidx);
  }
}

H5::Group HDF5DCOWriteFunctor::createPath(const std::string& path)
{
  try {
    // specify HDF5 exceptions to not print & abort
    H5::Exception::dontPrint();

    return file.lock()->openGroup(path);
    // no further action needed, group exists
  }
  catch(const H5::FileIException& e) {

    // path does not exist. is there a subpath?
    size_t idxs = path.rfind("/");

    if (idxs != 0) {
      // parent path found recursively call to check
      createPath(path.substr(0, idxs));
    }

    try {
      // specify HDF5 exceptions to not print & abort
      H5::Exception::dontPrint();
      // the path
      return file.lock()->createGroup(path);
    }
    catch(const H5::Exception& e) {
      std::cerr << "Trying to create group, got " <<
        e.getDetailMsg() << std::endl;
      throw(e);
    }
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to open group, got " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

static bool isFixedSize(const H5::DataType& datatype);
static bool isFixedSize(const H5::DataType& datatype);

static bool isFixedSize(const H5::CompType& datatype)
{
  for (int ii = datatype.getNmembers(); ii--; ) {
    if (isFixedSize(datatype.getMemberDataType(ii))) return false;
  }
  return true;
}

static bool isFixedSize(const H5::DataType& datatype)
{
  if (datatype.getClass() == H5T_COMPOUND) {
    return isFixedSize(dynamic_cast<const H5::CompType&>(datatype));
  }
  if (datatype.getClass() == H5T_VLEN ||
      datatype.isVariableStr()) {
    return false;
  }
  return true;
}

void HDF5DCOWriteFunctor::
configureDataSet(unsigned idx,
                 const std::string& name, hsize_t offset,
                 const H5::DataType* datatype,
                 hsize_t ncols)
{
  try {
    H5::Exception::dontPrint();

    sets[idx].datatype = datatype;
    if (datatype == NULL) {
      // ignore this dataset!
      /* DUECA hdf5.

         The given dataset/datatype cannot be written, and is ignored. */
      W_XTR("Cannot write dataset " << basepath << name);
      return;
    }
    H5::DSetCreatPropList proplist;
    proplist.setLayout( H5D_CHUNKED );
    sets[idx].dspace_dims[1] = ncols;

    // determine datatype size
    size_t dsize = datatype->getSize();
    /* DUECA hdf5.

       Information on the configured dataset. */
    I_XTR("for " << basepath << " set=" << idx << " dsize=" << dsize <<
          " ncols=" << ncols << " chunksize=" << chunksize <<
          " bytes/chunk=" << chunksize*ncols*dsize);
    if (chunksize*ncols*dsize > 1024*1024) {
      /* DUECA hdf5.

         The dataset configuration has a very large chunk size! */
       W_XTR("for " << basepath << " set=" << idx <<
            " large bytes/chunk=" << chunksize*ncols*dsize);
    }
    hsize_t chunk_dims[2] = { chunksize, ncols };
    hsize_t max_dims[2] = { H5S_UNLIMITED, ncols };
    unsigned ndims = ncols == 1 ? 1 : 2;
    proplist.setLayout( H5D_CHUNKED );
    proplist.setChunk( ndims, chunk_dims );


    // memory space is 1-dimensional, array or simply size 1
    if (ncols == 1) {
      sets[idx].memspace = H5::DataSpace(H5S_SCALAR);
    }
    else {
      sets[idx].memspace.setExtentSimple(ndims, sets[idx].dspace_dims);
    }

    // file space is potentially 2-dimensional, initial size is the
    // chunk size
    sets[idx].filspace.setExtentSimple(ndims, chunk_dims, max_dims);

    if (compress) {
      proplist.setDeflate(6);
    }

    // find default values dataset access
    hid_t dapl = proplist.getId(); //H5Pcreate(H5P_DATASET_ACCESS);
    size_t rdcc_nelmts;
    size_t rdcc_nbytes;
    double rdcc_w0;
    H5Pget_chunk_cache(dapl, &rdcc_nelmts, &rdcc_nbytes, &rdcc_w0);

    if (isFixedSize(*datatype)) {
      // make it so only one chunk in memory at a time. The 32 is a guess
      rdcc_nelmts = 1;
      rdcc_nbytes = chunksize * ncols * datatype->getSize() + 32;
      rdcc_w0 = 1.0;
      /* DUECA hdf5.

         Information on the cache properties of the given dataset. */
      I_XTR("For " << basepath << '/' << name <<
            " setting cache properties ndata:" << rdcc_nelmts <<
            " bdata:" << rdcc_nbytes << " w0:" << rdcc_w0);
      if (rdcc_nbytes > 8388608) {
        /* DUECA hdf5.

           Warning on the large chunk size of a given dataset. */
        I_XTR("Warning large chunk; " << basepath << '/' << name <<
              " bdata:" << rdcc_nbytes);
      }
      H5Pset_chunk_cache(dapl, rdcc_nelmts, rdcc_nbytes, rdcc_w0);
    }
    else {
      // just modify the flush level, fully written = flush
      rdcc_w0 = 1.0;
      H5Pset_chunk_cache(dapl, rdcc_nelmts, rdcc_nbytes, rdcc_w0);
      /* DUECA hdf5.

         The given dataset has a variable size. */
      I_XTR("Variable size " << basepath << name);
    }

    sets[idx].dset =
      file.lock()->createDataSet(basepath + name, *datatype,
                                 sets[idx].filspace, proplist);

    sets[idx].ncols = ncols;
    sets[idx].offset = offset;
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to configure dataset " << name
              << ", got " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

HDF5DCOWriteFunctor::LogDataSet::LogDataSet() :
  ncols(1), datatype(NULL), offset(0)
{ dspace_dims[0] = 1; dspace_dims[1] = 1; }

void HDF5DCOWriteFunctor::LogDataSet::prepareRow(unsigned chunkidx,
                                                 unsigned chunksize, bool flush)
{
  try {
    H5::Exception::dontPrint();


    if (flush) {

      // flush to file
      dset.flush(H5F_SCOPE_LOCAL);
      // filspace.selectNone();

      // extend data set
      hsize_t newsize[2] = {chunkidx + chunksize, ncols};
      dset.extend(newsize);
      //filspace.extend(newsize);
      // match filespace
      //filspace = dset.getSpace();
      filspace.setExtentSimple(ncols == 1 ? 1:2, newsize);
    }

    hsize_t offsetf[2] = { chunkidx, 0};
    filspace.selectHyperslab(H5S_SELECT_SET, dspace_dims, offsetf);
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to prepare rows " << chunkidx
              << ", got " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

void HDF5DCOWriteFunctor::LogDataSet::finalize(unsigned finalsize)
{
  try {
    H5::Exception::dontPrint();

    hsize_t newsize[2] = {finalsize, ncols};
    dset.extend(newsize);
    dset.flush(H5F_SCOPE_LOCAL);
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to finalize to " << finalsize
              << ", got " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

void HDF5DCOWriteFunctor::prepareRow()
{
  bool flush = chunkidx && (chunkidx % chunksize == 0);
  for (unsigned idx = sets.size(); idx--; ) {
    sets[idx].prepareRow(chunkidx, chunksize, flush);
  }
  chunkidx++;
}

void HDF5DCOWriteFunctor::LogDataSet::writeNew(const void* data)
{
  if (datatype == NULL) return;
  union {const void* ptr; const char* data;} conv; conv.ptr = data;
  try {
    H5::Exception::dontPrint();
    dset.write(&conv.data[offset], *datatype, memspace, filspace);
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to write new data "
              << ", got " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

void HDF5DCOWriteFunctor::LogDataSet::
writeNew(const void* data, hsize_t chunkidx,
         const std::string& dum)
{
  union {const char* ptr; const std::string* val;} conv;
  conv.ptr = reinterpret_cast<const char*>(data)+offset;
  try {
    H5::Exception::dontPrint();
    dset.write(conv.val, *datatype, memspace, filspace);
  }
  catch(const H5::Exception& e) {
    std::cerr << "Trying to write a string "
              << ", got " << e.getDetailMsg() << std::endl;
    throw(e);
  }
}

#if 0
    {
      if (datatype == NULL) return;
      union {const char* ptr; const V<D,Alloc>* val;} conv;
      conv.ptr = reinterpret_cast<const char*>(data)+offset;
      const D cpointers[conv.val->size()];
      unsigned idx = 0;
      for (typename V<D,Alloc>::const_iterator ii = conv.val->begin();
           ii != conv.val->end(); ii++) {
        cpointers[idx++] = (*ii);
      }
      dset.write(cpointers, *datatype, memspace, filspace);
    }
#endif

ENDHDF5LOG;
