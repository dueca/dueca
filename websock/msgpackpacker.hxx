/* ------------------------------------------------------------------   */
/*      item            : msgpackpacker.hxx
        made by         : Rene van Paassen
        date            : 240516
        category        : header file
        description     :
        changes         : 240516 first version
        language        : C++
        copyright       : (c) 2024 René van Paassen
        license         : EUPL-1.2
*/

#pragma once
// https://stackoverflow.com/questions/44725299/messagepack-c-how-to-iterate-through-an-unknown-data-structure

#include "WebsockExceptions.hxx"
#include <dueca/CommObjectElementReader.hxx>
#include <dueca/CommObjectElementWriter.hxx>
#include <dueca/CommObjectMemberArity.hxx>
#include <dueca/CommObjectReader.hxx>
#include <dueca/CommObjectWriter.hxx>
#include <dueca/DataTimeSpec.hxx>
#include <dueca/Dstring.hxx>
#include <dueca/smartstring.hxx>
#include <dueca_ns.h>
#include <map>
#include <stdexcept>
#define MSGPACK_USE_BOOST
#include <DCOTypeIndex.hxx>
#include <dueca/debug.h>
#include <msgpack.hpp>

#if MSGPACK_VERSION_MAJOR >= 3
#define DUECA_WEBSOCK_WITH_MSGPACK
#endif

#ifdef DUECA_WEBSOCK_WITH_MSGPACK
#include <msgpack/v3/null_visitor_decl.hpp>
#include <msgpack/v3/object_fwd_decl.hpp>

// need a version >= 3 for the websock-msgpack code

DUECA_NS_START;
WEBSOCK_NS_START;

/** Type definition for conversion of c++ data to a byte stream. */
typedef msgpack::packer<std::ostream> mwriter_t;

/** Helper function for coding a DCO object

    @param writer  Stream writing msgpack
    @param reader  DCO object reading class
  */
void code_dco(msgpack::packer<std::ostream> &writer,
              const CommObjectReader &reader);

// typedef std::map<std::string, msgpack::type::variant_ref> mainmap_t;
typedef std::map<std::string, msgpack::object> mainmap_t;
typedef std::vector<msgpack::object> mainvec_t;

/** Helper function for decoding a DCO object.

    @param obj  Parsed msgpack structure
    @param dco  DCO object for data target.
  */
void decode_dco(const mainmap_t &obj, CommObjectWriter &dco);

/** Class that can pack DCO objects and others into a msgpack message. */
struct msgpackpacker
{
  /** Writer object */
  msgpack::packer<std::ostream> writer;

  /** Constructor

      @param buffer  writer object for collecting packed data
    */
  msgpackpacker(std::ostream &buffer);

  /** Start a new structure,

      @param n   Number of members
    */
  inline void StartObject(size_t n) { writer.pack_map(n); }

  /** Complete a structure. 
  
      Does nothing, for compatibility with JSON packing.
   */
  inline void EndObject() {}

  /** Write a structure key, as a string.
  
      @param k  Key value  
   */
  inline void Key(const char *k)
  {
    writer.pack_str(strlen(k));
    writer.pack_str_body(k, strlen(k));
  }

  /** Start an array
  
      @param n   Number of members
   */
  inline void StartArray(size_t n) { writer.pack_array(n); }

  /** Complete an array. 
  
      Does nothing, for compatibility with JSON packing.
   */
  inline void EndArray() {}

  inline void String(const char *s)
  {
    writer.pack_str(strlen(s));
    writer.pack_str_body(s, strlen(s));
  }

  inline void String(const std::string &s)
  {
    writer.pack_str(s.size());
    writer.pack_str_body(s.c_str(), s.size());
  }

  inline void Int(int i) { writer.pack_int(i); }

  inline void Uint(unsigned i) { writer.pack_uint32(i); }

  inline void Bool(bool b)
  {
    if (b) {
      writer.pack_true();
    }
    else {
      writer.pack_false();
    }
  }

  inline void Double(double d) { writer.pack_double(d); }

  /** Pack a DCO object. */
  inline void dco(const DCOReader &r) { code_dco(writer, r); }

  inline void EndLine() {}

  /** websockets opcode for a packet, in this case binary */
  static inline unsigned char OpCode() { return 130; }
};

/** Unpack a parse msgpack object to variables or a DCO object. */
struct msgpackunpacker
{
  /** Object handle, must remain alive while unpacking the object */
  msgpack::object_handle oh;

  /** Object itself */
  msgpack::object obj;

  /** Map with immediate objects */
  mainmap_t doc;

  msgpackunpacker(const std::string &s);

  inline DataTimeSpec getStreamTime() const
  {
    auto it = doc.at("tick");
    return DataTimeSpec(it.as<std::vector<TimeTickType>>()[0],
                        it.as<std::vector<TimeTickType>>()[1]);
  }

  inline DataTimeSpec getTime() const
  {
    auto it = doc.at("tick");
    return DataTimeSpec(it.as<TimeTickType>());
  }

  inline void codedToDCO(DCOWriter &wr) const
  {
    auto data = doc.at("data");
    auto args = data.as<mainmap_t>();
    decode_dco(args, wr);
  }

  inline bool findMember(const char *name, std::string &result)
  {
    try {
      auto im = doc.at(name);
      result = im.as<std::string>();
      return true;
    }
    catch (const std::out_of_range &e) {
      return false;
    }
  }

  inline bool findMember(const char *name, bool &result)
  {
    try {
      auto im = doc.at(name);
      result = im.as<bool>();
      return true;
    }
    catch (const std::out_of_range &e) {
      return false;
    }
  }
};

WEBSOCK_NS_END;
DUECA_NS_END;
#endif

#include <dueca/undebug.h>
