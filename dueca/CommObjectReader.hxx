/* ------------------------------------------------------------------   */
/*      item            : CommObjectReader.hxx
        made by         : Rene van Paassen
        date            : 131202
        category        : header file
        description     :
        changes         : 131202 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectReader_hxx
#define CommObjectReader_hxx

#include <dueca_ns.h>
#include <CommObjectReaderWriter.hxx>
#include <CommObjectExceptions.hxx>
#include <DataReader.hxx>

DUECA_NS_START;

/* advance declaration*/
class ElementReader;
class ChannelReadToken;

/** Class to access data in a channel without access to the class of that data.

    Note that using a DataReader is in many cases much more efficient,
    the CommObjectReader is only applicable for generic or
    data-agnostic reading.

    Given the name of one of the Communication object's members, or
    the index of a data member (as returned by the
    DataClassRegistry::getMemberIndex call), an ElementReader can be
    created.
*/
class CommObjectReader: public CommObjectReaderWriter
{
protected:
  /** Pointer to the currently accessed object, NULL if not used */
  const void* obj;

public:
  /** Constructor, for testing purposes, and for recursively accessing
      complex objects.

      @param classname Type of data; must match, or the result is
                       nonsense!
      @param obj       Pointer to the object.
  */
  CommObjectReader(const char* classname, const void* obj = NULL);

  /** Return an element accessor based on the element name

      @param ename     Name of the data member  */
  ElementReader operator [] (const char* ename);

  /** Return an element accessor based on index

      @param i         Index of the data member */
  ElementReader operator [] (unsigned i);

  /** Destructor */
  ~CommObjectReader();
};



/** Introspective access to data in a channel.

    The DCOReader accesses the data in a channel. These objects
    should be created on the stack, and when they go out of scope, the
    access to the channel is released again.

    This variant is for generic, introspective access, using a
    @ref DataReader is much more efficient when you can program for the
    data type.

    By default, the reader accesses the latest datapoint in the
    channel that is not newer/later than the time specification
    (start) or time provided. If that is not what you want, test for
    the data time specification.
*/
class DCOReader: public CommObjectReader
{
  /** Time specification for the access */
  DataTimeSpec                ts_data;

  /** Origin of the current entry */
  GlobalId                    data_origin;

  /** access token */
  ChannelReadToken           &token;

  /** request time */
  TimeTickType                ts_request;

public:
  /** Constructor. Note that these objects are light-weight, and meant to be
      constructed (on the stack) and discarded.

      @param classname Type of data to be read; must match the data type in the
                       entry accessed with the read token.
      @param token     Read token.
      @param ts        Time specification. Accessed data point will not be newer
                       than ts.getValidityStart()
  */
  DCOReader(const char* classname,
            ChannelReadToken &token, const DataTimeSpec& ts);

  /** Constructor with TimeSpec.

      @param classname Type of data to be read; must match the data type in the
                       entry accessed with the read token.
      @param token     Read token.
      @param ts        Time specification. Accessed data point will not be newer
                       than ts.getValidityStart()
 */
  DCOReader(const char* classname,
            ChannelReadToken &token, const TimeSpec& ts);

  /** Constructor with time tick.

      @param classname Type of data to be read; must match the data type in the
                       entry accessed with the read token.
      @param token     Read token.
      @param ts        Time tick. Accessed data point will not be newer than
                       the tick. Note that the default (not specifying
                       this parameter) simply gives you the latest
                       data in the channel if JumpToMatchTime is
                       selected for the read token.
 */
  DCOReader(const char* classname, ChannelReadToken &token,
            TimeTickType ts=MAX_TIMETICK);

  /** Destructor */
  ~DCOReader();

  /** Return the time specification of the data. */
  inline const DataTimeSpec& timeSpec()
  {
    // access();
    return this->ts_data;
  }

  /** Return the origin/sender of the data */
  inline const GlobalId& origin()
  {
    // access();
    return this->data_origin;
  }

private:
  /** Copying is not possible. */
  DCOReader(const DCOReader&);

  /** Nor is assignment. */
  DCOReader& operator = (const DCOReader& );

  /** And new is certainly forbidden! */
  static void* operator new(size_t s);

  /** initiate access */
  void access();
};


DUECA_NS_END;
#endif
