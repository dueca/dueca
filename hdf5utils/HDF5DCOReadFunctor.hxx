/* ------------------------------------------------------------------   */
/*      item            : HDF5DCOReadFunctor.hxx
        made by         : Rene van Paassen
        date            : 170427
        category        : header file
        api             : DUECA_API
        description     :
        changes         : 170427 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef HDF5DCOReadFunctor_hxx
#define HDF5DCOReadFunctor_hxx

#include "HdfLogNamespace.hxx"
#include <DCOFunctor.hxx>
#include <DataTimeSpec.hxx>
#include <H5Cpp.h>
#include <vector>
#include <boost/weak_ptr.hpp>
#include <map>
#include <fixvector.hxx>
#include <string>
#include <map>

STARTHDF5LOG;

USING_DUECA_NS;

/** For reading DCO objects elements from a HDF file. Elements can
    contain either scalars, fixed-length vectors or variable length
    vectors of data. From a base path, each element gets a dataset
    with its name.
*/
class HDF5DCOReadFunctor: public DCOFunctor
{
  /** Pointer to the file */
  boost::weak_ptr<H5::H5File> file;

protected:

  /** Index in the chunk sets */
  size_t                      readidx;

  /** Flag to indicate an advance in data reading */
  bool                        advance;

  /** Number of rows available */
  size_t                      nrows;

  /** read ticks too (only for leaf child, not for parent objects) */
  bool                        readticks;

protected:
  /** Organize data per element */
  struct LogDataSet {

    /** Offset location */
    hsize_t                   offset_dims[2];

    /** Single row dimension */
    hsize_t                   row_dims[2];

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

    /** Before reading an object, prepare all indices */
    void prepareRow(unsigned readidx);

    /** With string */
    void readObjectPart(void* data, const std::string& dum);

    /** With fixed size vector of strings */
    template <size_t N>
    void readObjectPart(void* data,
                        const dueca::fixvector<N,std::string>& dum)
    {
      union {char* ptr; dueca::fixvector<N,std::string>* val;} conv;
      conv.ptr = reinterpret_cast<char*>(data)+offset;
      hsize_t ddims[2] = { 1, 1 };
      H5::DataSpace dataspace(H5S_SCALAR);
      for (hsize_t ii = N; ii--; ) {
        offset_dims[1] = ii;
        filspace.selectHyperslab(H5S_SELECT_SET, ddims, offset_dims);
        dset.read((*conv.val)[ii], *datatype, memspace, filspace);
      }
    }

    /** With iterable and c++ strings */
    template<typename Alloc, template <typename, typename> class V >
    void readObjectPart(void* data, V<std::string, Alloc>& dum)
    {
      union { char* ptr; V<std::string,Alloc>* val;} conv;
      conv.ptr = reinterpret_cast<char*>(data)+offset;

      try {
        H5::Exception::dontPrint();
        filspace.selectHyperslab(H5S_SELECT_SET, row_dims, offset_dims);
        struct {
          size_t len;
          char** str;
        } tmpdata;

        conv.val->clear();
        dset.read(&tmpdata, *datatype, memspace, filspace);
        for (unsigned ii = 0; ii < tmpdata.len; ii++) {
          conv.val->push_back(std::string(tmpdata.str[ii]));
        }
        H5::DataSet::vlenReclaim(&tmpdata, *datatype, memspace);
      }
      catch(const H5::Exception& e) {
        std::cerr << "Trying to variable no of strings "
                  << ", got " << e.getDetailMsg() << std::endl;
        throw(e);
      }
    }

    /** With iterable and anything but strings, requires a copy */
    template<typename Alloc, typename D, template <typename,typename> class V>
    void readObjectPart(void* data, V<D,Alloc>& dum)
    {
      if (datatype == NULL) return;
      union { char* ptr; V<D,Alloc>* val;} conv;
      conv.ptr = reinterpret_cast<char*>(data)+offset;
      try {
        H5::Exception::dontPrint();
        filspace.selectHyperslab(H5S_SELECT_SET, row_dims, offset_dims);
        struct {
          size_t len;
          D* object;
        } tmpdata;

        conv.val->clear();
        dset.read(&tmpdata, *datatype, memspace, filspace);
        for (unsigned ii = 0; ii < tmpdata.len; ii++) {
          conv.val->push_back(tmpdata.object[ii]);
        }
        H5::DataSet::vlenReclaim(&tmpdata, *datatype, memspace);
      }
      catch(const H5::Exception& e) {
        std::cerr << "Trying to variable no of objects "
                  << ", got " << e.getDetailMsg() << std::endl;
        throw(e);
      }
    }

  public:

    /** all others, direct type */
    template <typename T>
    void readObjectPart(void* data, const T& dum)
    {
      this->readObjectPart(data);
    }

    /** reading back-end */
    void readObjectPart(void* data);
  };

  /** One set per element/member of the DCO object. If applicable, the
      last set is for the time tick. */
  std::vector<LogDataSet> sets;

  /** Base path for writing the data; under this a set of 1 + 2d
      vectors will be defined. */
  std::string             basepath;

protected:

  /** Configure a dataset */
  void configureDataSet(unsigned idx,
                        const std::string& name, hsize_t offset,
                        const H5::DataType* datatype, hsize_t ncols);
public:

  /** Local function to flush/write and prepare for next present data

      @param nextrow    If true; and advance the index, so
                        that a next getTick will read new data.
      @returns          Time associated with the current data row
   */
  TimeTickType getTick(bool nextrow=false);

  /** Constructor */
  HDF5DCOReadFunctor(boost::weak_ptr<H5::H5File>& file,
                     const std::string& path,
                     size_t nelts, bool readticks);

  /** obtain the label attached to the data */
  const std::string& getLabel();

  /** Destructor */
  virtual ~HDF5DCOReadFunctor();
};

ENDHDF5LOG;

#endif
