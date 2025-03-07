/* ------------------------------------------------------------------   */
/*      item            : DataSetSubsidiary.hh
        made by         : Rene' van Paassen
        date            : 980710
        category        : header file
        description     :
        changes         : 980710 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataSetSubsidiary_hxx
#define DataSetSubsidiary_hxx

#include "DataSetConverter.hxx"
#include <dueca_ns.h>
DUECA_NS_START

/** Templated class for conversion of data to and from packed
    format. Implements the interface given by DataSetConverter. */
template<class T>
class DataSetSubsidiary: public DataSetConverter
{
public:
  /** Constructor. */
  DataSetSubsidiary();

  /** Destructor. */
  ~DataSetSubsidiary();

  /** Unpack data from storage, into the location given by set. */
  void unPackData(AmorphReStore & source, void* data) const;

  /** Pack data into storage. */
  void packData(AmorphStore& target, const void* data) const;

  /** Pack the difference between two objects. */
  void packDataDiff(AmorphStore& target, const void* data,
                    const void* ref) const;

  /** Create an object and fill it. */
  void* create(AmorphReStore& source) const;

  /** Create an object and fill it. */
  void* createDiff(AmorphReStore& source, const void* ref) const ;

  /** Clone an object */
  void* clone(const void* ref=0) const;

  /** Delete one object. */
  void delData(const void* data) const;

  /** Get the length of the data in unpacked format. */
  int getDataLength() const;

  /** Check if two objects are identical */
  bool areEqual(const void* d1, const void* d2) const;

  /** Get the magic number matching a data class type
      @returns           a "magic" number, hashed out of class definition
   */
  uint32_t getMagic() const;

  /** Get the memory/alloc size of a data class basic object
      @returns           size of (T)
  */
  size_t size() const;

  /** Return the class name */
  const char* getClassname() const;
};
DUECA_NS_END

#endif


//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#ifndef DataSetSubsidiary_ii
#define DataSetSubsidiary_ii

#include "AmorphStore.hxx"

DUECA_NS_START
template<class T>
DataSetSubsidiary<T>::DataSetSubsidiary()
{
  // nothing
}

template<class T>
DataSetSubsidiary<T>::~DataSetSubsidiary()
{
  // nothing
}

template<class T> void DataSetSubsidiary<T>::
unPackData(AmorphReStore& source, void* data) const
{
  ::unPackData(source, *(T*)(data));
}

template<class T> void DataSetSubsidiary<T>::
packData(AmorphStore& target, const void* data) const
{
  ::packData(target, *(const T*)(data));
}

template<class T> void DataSetSubsidiary<T>::
packDataDiff(AmorphStore& target, const void* data, const void* ref) const
{
  ::packDataDiff(target, *reinterpret_cast<const T*>(data),
                 *reinterpret_cast<const T*>(ref));
}

template<class T> void* DataSetSubsidiary<T>::
create(AmorphReStore& source) const
{
  return new T(source);
}

template<class T> void* DataSetSubsidiary<T>::
createDiff(AmorphReStore& source, const void* ref) const
{
  T* data = (ref == NULL) ? new T() : new T(*reinterpret_cast<const T*>(ref));

  ::unPackDataDiff(source, *reinterpret_cast<T*>(data));
  return data;
}

template<class T> void* DataSetSubsidiary<T>::
clone(const void* ref) const
{
  T* data = (ref == NULL) ? new T() : new T(*reinterpret_cast<const T*>(ref));
  return data;
}

template<class T> void DataSetSubsidiary<T>::delData(const void* data) const
{
  delete reinterpret_cast<const T*>(data);
}

template<class T> int
DataSetSubsidiary<T>::getDataLength() const
{
  return sizeof(T);
}

template<class T> bool
DataSetSubsidiary<T>::areEqual(const void* d1, const void* d2) const
{
  return *reinterpret_cast<const T*>(d1) == *reinterpret_cast<const T*>(d2);
}

template<class T> uint32_t DataSetSubsidiary<T>::
getMagic() const
{
  return T::magic_check_number;
}

template<class T> size_t DataSetSubsidiary<T>::size() const
{
  return sizeof(T);
}

template<class T> const char* DataSetSubsidiary<T>::
getClassname() const
{
  return getclassname<T>();
}

DUECA_NS_END
#endif
#endif
