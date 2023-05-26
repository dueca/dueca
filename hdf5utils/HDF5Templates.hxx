/* ------------------------------------------------------------------   */
/*      item            : HDF5Templates.hxx
        made by         : Rene van Paassen
        date            : 170325
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 170325 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2 - Rene van Paassen
*/

#ifndef HDF5Templates_hxx
#define HDF5Templates_hxx

#include <H5Cpp.h>
#include <inttypes.h>
#include <fixvector.hxx>
#include <fixvector_withdefault.hxx>
#include <varvector.hxx>
#include <limvector.hxx>
#include <string>
#include <Dstring.hxx>
#include <dueca/smartstring.hxx>
#include <dueca_ns.h>
#include <map>

DUECA_NS_START;

/** @file HDF5Templates.hxx 

    Template functions and classes to extract HDF5-relevant information about
    different data types. */

/** Return the HDF5 datatype.

 - known standard types (see below) are given by template
   specialization versions

 - DCO types are given by the generic case, assumption of a member
   function getHdf5DataType; this member is only possible if the DCO
   type is limited to members that do not use dynamic memory, with the
   exception of varvector. In that case the DCO type can be nested
   (possibly in containers) in other DCO types.

 - Enumerated types generated with DCO code generation

 - all these types must be directly accessible through the memory in a
   stuct-like compound. This means it will fail for containers like
   std::vector, std::map, std::list, will fail for string. DUECA's
   varvector, limvector, fixvector have been made to comply, and
   dueca's fixed-length strings are seen as c-style strings, and
   therefore also compatible.

 - can fail to compile for a DCO type, in that case do not take the
   option hdf5nest but hfd5
*/
template<typename T> const H5::DataType* get_hdf5_type(const T& t);

/** Return the HDF5 datatype. */
template<typename T> const H5::DataType* get_hdf5_type(T& t);

/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const double& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const float& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const int32_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const uint32_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const int64_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const uint64_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const int16_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const uint16_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const unsigned char& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const char& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const bool& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const std::string& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(const dueca::smartstring& t);

/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(double& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(float& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(int32_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(uint32_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(int64_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(uint64_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(int16_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(uint16_t& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(unsigned char& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(char& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(bool& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(std::string& t);
/** Specialization for known type */
template<> const H5::DataType* get_hdf5_type(dueca::smartstring& t);

/** Variant without arguments, but with typename as template parameter */
template<typename T>
const H5::DataType* get_hdf5_type();

/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<double>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<float>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<int32_t>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<uint32_t>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<int64_t>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<uint64_t>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<int16_t>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<uint16_t>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<unsigned char>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<char>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<bool>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<std::string>();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<float>();

/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<dueca::Dstring<8> >();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<dueca::Dstring<16> >();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<dueca::Dstring<32> >();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<dueca::Dstring<40> >();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<dueca::Dstring<64> >();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<dueca::Dstring<128> >();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<dueca::Dstring<256> >();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<dueca::Dstring<236> >();
/** Variant without arguments, but with typename as template parameter */
template<> const H5::DataType* get_hdf5_type<dueca::smartstring>();

/** Last template, for a fixed-length string */
template<unsigned N> const H5::DataType* get_hdf5_type(const Dstring<N>& t)
{ return get_hdf5_type<Dstring<N> >(); }
/** Last template, for a fixed-length string */
template<unsigned N> const H5::DataType* get_hdf5_type(Dstring<N>& t)
{ return get_hdf5_type<Dstring<N> >(); }


/** HDF5 type for common container */
template<size_t N, typename T>
const H5::DataType* get_hdf5_type(const dueca::fixvector<N,T>& t)
{
  static hsize_t dims[] = { N };
  static H5::ArrayType data_type(*get_hdf5_type<T>(), 1, dims);
  return &data_type;
}
/** HDF5 type for common container */
template<size_t N, typename T>
const H5::DataType* get_hdf5_type(dueca::fixvector<N,T>& t)
{
  static hsize_t dims[] = { N };
  static H5::ArrayType data_type(*get_hdf5_type<T>(), 1, dims);
  return &data_type;
}

template<size_t N, typename T, int DEFLT, unsigned BASE>
const H5::DataType* get_hdf5_type(const dueca::fixvector_withdefault<N,T,DEFLT,BASE>& t)
{
  static hsize_t dims[] = { N };
  static H5::ArrayType data_type(*get_hdf5_type<T>(), 1, dims);
  return &data_type;
}
/** HDF5 type for common container */
template<size_t N, typename T, int DEFLT, unsigned BASE>
const H5::DataType* get_hdf5_type(dueca::fixvector_withdefault<N,T,DEFLT,BASE>& t)
{
  static hsize_t dims[] = { N };
  static H5::ArrayType data_type(*get_hdf5_type<T>(), 1, dims);
  return &data_type;
}

/** HDF5 type for common container */
template<typename T>
const H5::DataType* get_hdf5_type(const dueca::varvector<T>& t)
{
  static H5::VarLenType data_type(get_hdf5_type<T>());
  return &data_type;
}
/** HDF5 type for common container */
template<typename T>
const H5::DataType* get_hdf5_type(dueca::varvector<T>& t)
{
  static H5::VarLenType data_type(get_hdf5_type<T>());
  return &data_type;
}

/** HDF5 type for common container */
template<size_t N, typename T>
const H5::DataType* get_hdf5_type(const dueca::limvector<N,T>& t)
{
  static H5::VarLenType data_type(get_hdf5_type<T>());
  return &data_type;
}
/** HDF5 type for common container */
template<size_t N, typename T>
const H5::DataType* get_hdf5_type(dueca::limvector<N,T>& t)
{
  static H5::VarLenType data_type(get_hdf5_type<T>());
  return &data_type;
}

/** HDF5 type for properly defined wrappable DCO objects */
template<class T>
const H5::DataType* get_hdf5_type()
{
  return T::getHDF5DataType();
}

/** elt_type and elt_length variants are used in non-nested packing;
    DCO members are packed as arrays. This gives more possibilities,
    and std::list, std::vector can also be stored.

    If get_hdf5_elt_type returns 0, the object can not be stored as
    such, and is ignored when writing members of a DCO object

    Caveat: There are many options and combinations possible, and
    before relying on storing variable-length lists with
    variable-length objects, please check the resulting logging and
    the computation times needed.
*/
template<class T> const H5::DataType* get_hdf5_elt_type(const T&);
template<class T> const H5::DataType* get_hdf5_elt_type(T&);

/** for fixed-length vector, becomes 2d dataset */
template<size_t N, class D>
const H5::DataType* get_hdf5_elt_type(const dueca::fixvector<N,D>& d)
{ return get_hdf5_type<D>(); }
/** for fixed-length vector, becomes 2d dataset */
template<size_t N, class D>
const H5::DataType* get_hdf5_elt_type(dueca::fixvector<N,D>& d)
{ return get_hdf5_type<D>(); }

/** for fixed-length vector, becomes 2d dataset */
template<size_t N, class D, int DEFLT, unsigned BASE>
const H5::DataType* get_hdf5_elt_type(const dueca::fixvector_withdefault<N,D,DEFLT,BASE>& d)
{ return get_hdf5_type<D>(); }
/** for fixed-length vector, becomes 2d dataset */
template<size_t N, class D, int DEFLT, unsigned BASE>
const H5::DataType* get_hdf5_elt_type(dueca::fixvector_withdefault<N,D,DEFLT,BASE>& d)
{ return get_hdf5_type<D>(); }

/** Ignore std::map if trying to write it nested */
template<typename K, typename T>
const H5::DataType* get_hdf5_type(const std::map<K,T>& d)
{ return NULL; }
/** Ignore std::map if trying to write it nested */
template<typename K, typename T>
const H5::DataType* get_hdf5_type(std::map<K,T>& d)
{ return NULL; }

/* when directly lifted from struct, std::string does not work */
//const H5::DataType* get_hdf5_type(const std::string& d);

/** An element, it must be writeable */
const H5::DataType* get_hdf5_elt_type(const std::string& d);
/** An element, it must be writeable */
const H5::DataType* get_hdf5_elt_type(std::string& d);


/** when directly lifted from struct, std::string does not work */
const H5::DataType* get_hdf5_type(const smartstring& d);
/** when directly lifted from struct, std::string does not work */
const H5::DataType* get_hdf5_type(smartstring& d);

/** as element, it must be writeable */
const H5::DataType* get_hdf5_elt_type(const smartstring& d);
/** as element, it must be writeable */
const H5::DataType* get_hdf5_elt_type(smartstring& d);

/** std::map works as member, written as extensible array with
    key,val pairs. These must be convertible in one go */
template<typename K, typename T>
const H5::DataType* get_hdf5_elt_type(const std::map<K,T>& d)
{
  static H5::CompType *data_type = NULL;
  static H5::VarLenType *arr_type = NULL;
  static bool once = true;
  typedef std::pair<K,T> pairlike;
  //struct pairlike { K first; T second; };
  if (once) {
    once = false;
    pairlike example;
    if (dueca::get_hdf5_type(example.first) == NULL ||
        dueca::get_hdf5_type(example.second) == NULL) return NULL;
    data_type = new H5::CompType(sizeof(typename std::map<K,T>::value_type));
    data_type->insertMember
      ("key", HOFFSET(pairlike, first),
       *dueca::get_hdf5_type(example.first));
    data_type->insertMember
      ("val", HOFFSET(pairlike, second),
       *dueca::get_hdf5_type(example.second));
    arr_type = new H5::VarLenType(data_type);
  }
  return arr_type;
}

/** std::map works as member, written as extensible array with
    key,val pairs. These must be convertible in one go */
template<typename K, typename T>
const H5::DataType* get_hdf5_elt_type(std::map<K,T>& d)
{
  const std::map<K,T>& _d = d;
  return get_hdf5_elt_type(_d);
}

/** generic element type */
template<typename T>
const H5::DataType* get_hdf5_elt_type(const T& o)
{ return get_hdf5_type(o); }

/** generic element type */
template<typename T>
const H5::DataType* get_hdf5_elt_type(T& o)
{ return get_hdf5_type(o); }

/** Return the length of a fixed-size vector */
template<size_t N, class D>
const hsize_t get_hdf5_elt_length(const dueca::fixvector<N,D>& d)
{ return N; }

/** Return the length of a fixed-size vector */
template<size_t N, class D, int DEFLT, unsigned BASE>
const hsize_t get_hdf5_elt_length(const dueca::fixvector_withdefault<N,D,DEFLT,BASE>& d)
{ return N; }

/** Return the length of any other element */
template<class T>
const hsize_t get_hdf5_elt_length(const T&)
{ return 1; }

/** stl-type iterable with std::string */
template<typename Alloc, template <typename, typename> class V >
const H5::DataType* get_hdf5_elt_type(const V<std::string, Alloc>& t)
{
  static H5::VarLenType data_type(get_hdf5_elt_type(std::string()));
  return &data_type;
}

/** stl-type iterable with std::string */
template<typename Alloc, template <typename, typename> class V >
const H5::DataType* get_hdf5_elt_type(V<std::string, Alloc>& t)
{
  static H5::VarLenType data_type(get_hdf5_elt_type(std::string()));
  return &data_type;
}

/** stl-type iterable with other stuff */
template<typename Alloc, typename D, template <typename,typename> class V>
const H5::DataType* get_hdf5_elt_type(const V<D, Alloc>& t)
{
  static H5::VarLenType data_type(get_hdf5_type(D()));
  return &data_type;
}

/** stl-type iterable with other stuff */
template<typename Alloc, typename D, template <typename,typename> class V>
const H5::DataType* get_hdf5_elt_type(V<D, Alloc>& t)
{
  static H5::VarLenType data_type(get_hdf5_type(D()));
  return &data_type;
}

/** Generic datatype as very last, works for DCO objects */
template<class T>
const H5::DataType* get_hdf5_type(const T& t)
{
  return T::getHDF5DataType();
}

/** Generic datatype as very last, works for DCO objects */
template<class T>
const H5::DataType* get_hdf5_type(T& t)
{
  return T::getHDF5DataType();
}


DUECA_NS_END;

#endif
