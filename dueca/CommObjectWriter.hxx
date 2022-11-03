/* ------------------------------------------------------------------   */
/*      item            : CommObjectWriter.hxx
        made by         : Rene van Paassen
        date            : 131202
        category        : header file
        description     :
        changes         : 131202 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectWriter_hxx
#define CommObjectWriter_hxx

#include <dueca_ns.h>
#include <CommObjectReaderWriter.hxx>
#include <TimeSpec.hxx>
#include <SimTime.hxx>

DUECA_NS_START;

class ElementWriter;
class ChannelWriteToken;
namespace ddff { class DDFFDataRecorder; }

/** Class to access a communication object. Is returned by querying a
    channel, the most common way to get these is by creating a DCOWriter.

    This class implements introspection-based access to DCO
    objects. If you have a void pointer and know the classname, this
    can also be created directly.

    Based on either the name of the data member, or on the index of a
    data member (as returned by the DataClassRegistry::getMemberIndex call)
    you can create an ElementWriter. With such a writer the members of
    the encapsulated object can be manipulated.
 */
class CommObjectWriter: public CommObjectReaderWriter
{
protected:
  /** Pointer to the currently accessed object, NULL if not used */
  void* obj;

public:
  /** Constructor */
  CommObjectWriter(const char* classname, void* obj = NULL);

  /** Return an element accessor based on the element name

      @param ename  Name of the data member
  */
  ElementWriter operator [] (const char* ename);

  /** Return an element accessor based on index

      @param i      Index of the data member
   */
  ElementWriter operator [] (unsigned i);

  /** Destructor */
  ~CommObjectWriter();

  /** assignment, needed for temporary copy MSGPACKtoDCO */
  CommObjectWriter& operator = (const CommObjectWriter& o);

private:
  // for DataRecorder
  friend class ddff::DDFFDataRecorder;

  /** Directly access the object pointer, for example when using a
      functor */
  inline void *getObjectPtr() { return obj; }
};


/** Introspective access to data in a channel.

    The DCOWriter accesses the data in a channel. These objects
    should be created on the stack, and when they go out of scope, the
    access to the channel is released again.

    A DCOWriter is an alternative to using a @ref DataWriter. This variant
    is for generic, introspective access, using a DataWriter is much
    more efficient when you can program for the data type.
*/
class DCOWriter: public CommObjectWriter
{
  /** Time specification for the access */
  DataTimeSpec                ts_write;

  /** access token */
  ChannelWriteToken          &token;

  /** Flag to remember success */
  bool                        a_ok;

public:
  /** Constructor. Note that these objects are light-weight, and meant to be
      constructed (on the stack) and discarded.

      @param classname Type of data to be read; must match the data type in the
                       entry accessed with the write token.
      @param token     Read token.
      @param ts        Time specification.
  */
  DCOWriter(const char* classname,
            ChannelWriteToken &token, const DataTimeSpec& ts);

  /** Constructor. Note that these objects are light-weight, and meant to be
      constructed (on the stack) and discarded. This version assumes the data
      type from the channel entry.

      @param token     Write token.
      @param ts        Time specification.
  */
  DCOWriter(ChannelWriteToken &token, const DataTimeSpec& ts);

  /** Constructor with time tick, for event writing. Note that these
      objects are light-weight, and meant to be constructed (on the
      stack) and discarded. This version assumes the data type from
      the channel entry.

      @param token     Write token.
      @param ts        Time specification, if omitted, takes current tick.
  */
  DCOWriter(ChannelWriteToken &token,
            TimeTickType ts = SimTime::getTimeTick());

  /** Destructor */
  ~DCOWriter();

  /** Flag a failure or error */
  inline void failed() { a_ok = false; }

private:
  /** Copying is not possible. */
  DCOWriter(const DCOWriter&);

  /** Nor is assignment. */
  DCOWriter& operator = (const DCOWriter& );

  /** And new is certainly forbidden! */
  static void* operator new(size_t s);
};

DUECA_NS_END;
#endif
