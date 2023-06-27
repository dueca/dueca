/* ------------------------------------------------------------------   */
/*      item            : smartstring.hxx
        made by         : Rene van Paassen
        date            : 210318
        category        : header file
        description     :
        changes         : 210318 first version
	api             : DUECA_API
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define smartstring_hxx
#pragma once
#include "DCOtoJSON.hxx"
#include "JSONtoDCO.hxx"
#include "DCOtoXML.hxx"
#include "XMLtoDCO.hxx"
#include "CommObjectWriter.hxx"
#include "CommObjectReader.hxx"
#include <exception>
#include "dueca_ns.h"

DUECA_NS_START;

class AmorphReStore;

/** Decode exception */
struct smartdecodeerror: public std::exception
{
  /** printed exception data */
  const char* reason;

  /** Constructor */
  smartdecodeerror(const char* reason) :
    std::exception(),
    reason(reason) {}

  /** Return the exception message */
  const char* what() const noexcept
  { return reason; }
};

/** "Smart" string class.

    This class adds facilities for coding and decoding DCO objects to
    and from JSON or XML in the string. This string class can be used
    in DCO objects, to provide an easy means to wrap other DCO objects
    or data, e.g., to provide initialization.
*/
class smartstring: public std::string
{
public:
  /** Helper struct to use an std::string as buffer for XML writing
      with pugi. */
  struct xml_string_writer: public pugi::xml_writer
  {
  private:
    /** Reference to the resulting output/buffer */
    std::string& result;
  public:

    /** Constructor.

        @param host    String for the result */
    xml_string_writer(std::string& host) : result(host) {}

    /** Write function used by pugixml */
    virtual void write(const void* data, size_t size)
    {
      result.append(static_cast<const char*>(data), size);
    }
  };

  /** Helper struct to create a json writer */
  struct json_string_writer:
    public rapidjson::Writer<
             rapidjson::GenericStringBuffer<rapidjson::UTF8<> > >
  {
  private:
    /** Reference to the resulting output/buffer */
    std::string& result;

    /** Reference to the resulting output buffer */
    rapidjson::StringBuffer doc;
  public:
    /** Constructor.

        @param host    String for the result */
    json_string_writer(std::string& host) : result(host), doc() {
      this->Reset(doc);
    }

    /** Update the associated string */
    virtual void force_sync()
    { result = doc.GetString(); }

    /** Destructor */
    ~json_string_writer()
    { result = doc.GetString(); }
  };


public:
  /** Constructor, default */
  smartstring();

  /** Constructor from char* */
  smartstring(const char* s);

  /** Constructor from char* and size limit */
  smartstring(const char* s, size_t n);

  /** Constructor with an AmorphReStore, for unpacking. */
  smartstring(AmorphReStore& s);

  /** Copy constructor */
  smartstring(const std::string& o);

  /** Copy constructor */
  smartstring(const smartstring& o);

  /** Fill constructor */
  smartstring(size_t n, char c);

  /** Assignment operator, with const char*. */
  smartstring & operator = (const char* o)
  { *dynamic_cast<std::string*>(this) = o;
    return * this; }

  /** Assignment operator, with another smartstring. */
  smartstring & operator = (const smartstring& o)
  { if (this != &o) { *dynamic_cast<std::string*>(this) = o; }
    return * this; }

  /** Assignment, from a standard string. */
  smartstring & operator = (const std::string& o)
  { if (this != &o) { *dynamic_cast<std::string*>(this) = o; }
    return * this; }

  /** Destructor */
  ~smartstring();

  /** Decode the XML in the string to the given DCO object

      @tparam DCO   Dueca Communication Object, or comparable
      @param obj    DCO object
   */
  template<class DCO>
  void decodexml(DCO& obj) const
  {
    CommObjectWriter cow(getclassname<DCO>(), &obj);
    pugi::xml_document doc;
    doc.load_string(this->c_str());
    XMLtoDCO(doc.child("object"), cow);
  }

  /** Encode the given DCO object as XML in this string

      @tparam DCO   Dueca Communication Object, or comparable
      @param obj    DCO object
   */
  template<class DCO>
  void encodexml(const DCO& obj)
  {
    CommObjectReader cor(getclassname<DCO>(), &obj);
    pugi::xml_document doc;
    DCOtoXML(doc, cor);
    xml_string_writer writer(*this);
    doc.save(writer);
  }

  /** Decode the JSON in the string to the given DCO object

      @tparam DCO   Dueca Communication Object, or comparable
      @param obj    DCO object
   */
  template<class DCO>
  void decodejson(DCO& obj) const
  {
    CommObjectWriter cow(getclassname<DCO>(), &obj);
    rapidjson::GenericDocument<rapidjson::UTF8<> > doc;
    rapidjson::ParseResult res = doc.Parse(this->c_str());
    if (!res) {
      throw smartdecodeerror("Cannot parse JSON document");
    }
    JSONtoDCO(doc, cow);
  }

  /** Encode the given DCO object as JSON in this string

      @tparam DCO   Dueca Communication Object, or comparable
      @param obj    DCO object
  */
  template<class DCO>
  void encodejson(const DCO& obj)
  {
    CommObjectReader cor(getclassname<DCO>(), &obj);
    rapidjson::StringBuffer doc;
    rapidjson::Writer<rapidjson::StringBuffer> writer(doc);
    DCOtoJSONstrict(writer, cor);
    *this = doc.GetString();
  }

};

DUECA_NS_END;

#include "msgpack-unstream-iter.hxx"
MSGPACKUS_NS_START;
template<typename S>
inline void msg_unpack(S& i0, const S& iend, dueca::smartstring& i)
{
  uint32_t len = unstream<S>::unpack_strsize(i0, iend);
  i.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    check_iterator_notend(i0, iend);
    i[ii] = *i0; ++i0;
  }
}
MSGPACKUS_NS_END;
