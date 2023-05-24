/* ------------------------------------------------------------------   */
/*      item            : HDF5DCOWriteFunctor.hxx
        made by         : Rene van Paassen
        date            : 170327
        category        : header file
        api             : DUECA_API
        description     :
        changes         : 170327 first version
                          171010 compression optional, some efficiency fixes
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef HDF5DCOWriteFunctor_hxx
#define HDF5DCOWriteFunctor_hxx

#include "HdfLogNamespace.hxx"
#include <DCOFunctor.hxx>
#include <DataTimeSpec.hxx>
#include <H5Cpp.h>
#include <vector>
#include <map>
#include <memory>
#include <fixvector.hxx>
#include <string>
#include <map>

STARTHDF5LOG;

USING_DUECA_NS;

/** Functor class to write DCO objects elements to a HDF file.

    Actually the name is a bit ambiguous, it reads from a channel, and
    then writes the data to file.

    A derived class, generated for each DCO class with the option
    "hdf5" or "hdf5nest" set, will implement the actual interaction
    between the channel and the data, through two
    DCOFunctor::operator() overrides from the DCOFunctor class.

    This middle class implements the interface on the HDF filing side.

    DCO objects will be defined to HDF structures; single members will
    be written as 1D arrays, with each data point corresponding to a
    DCO in the channel. Fixed length vectors are written as 2d
    arrays. Variable length vectors as 1D array of vectors. The time
    tick is written separately

    Due to this structure, DCO classes can be nested in (containers of)
    other DCO classes only if they are fixed-size. For this they need
    code generation with the hdf5nest option.
*/
class HDF5DCOWriteFunctor: public DCOFunctor
{
  /** Pointer to the file */
  std::weak_ptr<H5::H5File> file;

protected:
  /** Time span for writing */
  const dueca::DataTimeSpec  *startend;

  /** Write ticks? */
  bool                        writeticks;

  /** Compress dataset? */
  bool                        compress;

  /** Size of each data chunk, in rows */
  size_t                      chunksize;

  /** Index in the chunk sets */
  size_t                      chunkidx;

protected:
  /** Organize data per element */
  struct LogDataSet {

    /** Dataspace dimensions, for current (1 row) write. */
    hsize_t                   dspace_dims[2];

    /** Size of the data vector, one for each element, becomes columns
        (if one, scalar) */
    hsize_t                   ncols;

    /** Handle to the data in the file */
    H5::DataSet               dset;

    /** In-memory dataspace indices */
    H5::DataSpace             memspace;

    /** In-file dataspace indices */
    H5::DataSpace             filspace;

    /** Datatype */
    const H5::DataType       *datatype;

    /** Offset in the DCO object */
    unsigned                  offset;

    /** Default constructor. */
    LogDataSet();

    /** Before writing, prepare all indices */
    void prepareRow(unsigned chunkidx, unsigned chunksize, bool flush);

    /** With string */
    void writeNew(const void* data, hsize_t chunkidx, const std::string& dum);

    /** With fixed size vector of strings */
    template <size_t N>
    void writeNew(const void* data, hsize_t chunkidx,
                  const dueca::fixvector<N,std::string>& dum)
    {
      union {const char* ptr;
        const dueca::fixvector<N,std::string>* val;} conv;
      conv.ptr = reinterpret_cast<const char*>(data)+offset;
      hsize_t ddims[2] = { 1, 1 };
      H5::DataSpace dataspace(H5S_SCALAR);
      for (hsize_t ii = ncols; ii--; ) {
        hsize_t offsetf[2] = { chunkidx, ii};
        filspace.selectHyperslab(H5S_SELECT_SET, ddims, offsetf);
        dset.write((*conv.val)[ii], *datatype, dataspace, filspace);
      }
    }

    /** With iterable (stl vector & list) of c++ strings, needs
        copy of pointers */
    template<typename Alloc, template <typename, typename> class V >
    void writeNew(const void* data, hsize_t chunkidx,
                  const V<std::string, Alloc>& dum)
    {
      if (datatype == NULL) return;
      union {const char* ptr; const V<std::string,Alloc>* val;} conv;
      conv.ptr = reinterpret_cast<const char*>(data)+offset;
      struct {
        size_t len;
        const char** data;
      } varlen;
      const char* data2[conv.val->size()];
      varlen.data = data2;
      varlen.len = conv.val->size();
      unsigned idx = 0;
      for (typename V<std::string,Alloc>::const_iterator ii = conv.val->begin();
           ii != conv.val->end(); ii++) {
        varlen.data[idx++] = ii->c_str();
      }
      dset.write(&varlen, *datatype, memspace, filspace);
    }

    /** With map, treat as iterable with key and val types, assume pairs are
	appropriately packed */
    template<typename K, typename V>
    void writeNew(const void* data, hsize_t chunkidx,
		  const std::map<K, V>& dum)
    {
      if (datatype == NULL) return;
      union {const char* ptr;  const std::map<K, V>* val;} conv;
      conv.ptr = reinterpret_cast<const char*>(data)+offset;
      struct {
        size_t len;
	std::pair<K,V>* data;
      } varlen;
      std::pair<K,V> cpointers[conv.val->size()];
      varlen.len = conv.val->size();
      varlen.data = cpointers;
      unsigned idx = 0U;
      for (const auto& vals: *conv.val) {
	cpointers[idx++] = vals;
      }
      dset.write(&varlen, *datatype, memspace, filspace);
    }

  public:

    /** With iterable and anything but strings, requires a copy */
    template<typename Alloc, typename D, template <typename,typename> class V>
    void writeNew(const void* data, hsize_t chunkidx,
                  const V<D,Alloc>& dum)
    {
      if (datatype == NULL) return;
      union {const char* ptr; const V<D,Alloc>* val;} conv;
      conv.ptr = reinterpret_cast<const char*>(data)+offset;
      struct {
        size_t len;
        D* data;
      } varlen;
      D cpointers[conv.val->size()];
      varlen.len = conv.val->size();
      varlen.data = cpointers;
      unsigned idx = 0;
      for (typename V<D,Alloc>::const_iterator ii = conv.val->begin();
           ii != conv.val->end(); ii++) {
        cpointers[idx++] = *ii;
      }
      dset.write(&varlen, *datatype, memspace, filspace);
    }

    /** all objects are written as a direct type using HDF5 type from
        template

        @param data     Data pointer
        @param chunkidx Current write index
        @param dum      Reference to type
     */
    template <typename T>
    void writeNew(const void* data, hsize_t chunkidx, const T& dum)
    {
      this->writeNew(data);
    }

    /** Write data */
    void writeNew(const void* data);

    /** Flush the last bit */
    void finalize(unsigned finalsize);
  };

  /** One set per element */
  std::vector<LogDataSet> sets;

  /** Base path for writing the data; under this a set of 1 + 2d
      vectors will be defined. */
  std::string             basepath;

  /** Prepare the path */
  H5::Group createPath(const std::string& path);

protected:

  /** Configure a dataset */
  void configureDataSet(unsigned idx,
                        const std::string& name, hsize_t offset,
                        const H5::DataType* datatype, hsize_t ncols);

  /** Local function to flush/write and prepare for next present data */
  void prepareRow();

  /** Constructor */
  HDF5DCOWriteFunctor(std::weak_ptr<H5::H5File>& file,
                      const std::string& path, size_t chunksize,
                      const std::string& label,
                      size_t nelts, bool compress, bool writeticks,
                      const dueca::DataTimeSpec* startend);

public:
  /** Destructor */
  virtual ~HDF5DCOWriteFunctor();
};

ENDHDF5LOG;

#endif
