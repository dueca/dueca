/* ------------------------------------------------------------------   */
/*      item            : DataSetConverter.hh
        made by         : Rene' van Paassen
        date            : 980209
        category        : header file
        description     : DataSet converter. Base class + template, makes
                          and packs DataSets for the channels. In this
                          way a channel need not be typed.
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataSetConverter_hh
#define DataSetConverter_hh

#include <dueca_ns.h>
#include <inttypes.h>
#include <cstddef>

DUECA_NS_START

class AmorphStore;
class AmorphReStore;
class GenericEvent;

/** Base class for objects that convert (stream) data from and to net
    representation. */
class DataSetConverter
{
public:
  /** Constructor. */
  DataSetConverter();

  /** Destructor. */
  virtual ~DataSetConverter();

  /** Unpack data from storage, into the location given by set. */
  virtual void unPackData(AmorphReStore &source, void* data) const = 0;

  /** Pack data into storage. */
  virtual void packData(AmorphStore& target, const void* data) const = 0;

  /** Pack the difference between two objects. */
  virtual void packDataDiff(AmorphStore& target, const void* data,
                            const void* ref) const = 0;

  /** Create an object and fill it. */
  virtual void* create(AmorphReStore& source) const = 0;

  /** Create an object and fill it. */
  virtual void* createDiff(AmorphReStore& source, const void* ref) const = 0;

  /** Clone an object, if ref is NULL, a default
      initialization (which can be none!) is done */
  virtual void* clone(const void* ref) const = 0;

  /** Delete an object of said data type. */
  // virtual void delData(void* data) const = 0;

  /** Delete an const object of said data type. */
  virtual void delData(const void* data) const = 0;

  /** Get the length of the data in unpacked format. */
  virtual int getDataLength() const = 0;

  /** Check if two objects are identical */
  virtual bool areEqual(const void* d1, const void* d2) const = 0;

  /** Get the magic number matching a data class type
      @returns           a "magic" number, hashed out of class definition
   */
  virtual uint32_t getMagic() const = 0;

  /** Get the memory/alloc size of a data class basic object
      @returns           size of (T)
  */
  virtual size_t size() const = 0;

  /** Return the class name */
  virtual const char* getClassname() const = 0;
};

DUECA_NS_END
#endif

