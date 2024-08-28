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
#include <dueca/CommObjectElementWriter.hxx>
#include <dueca/CommObjectMemberArity.hxx>
#include <dueca/CommObjectWriter.hxx>
#include <dueca/DataTimeSpec.hxx>
#include <dueca/Dstring.hxx>
#include <dueca/smartstring.hxx>
#include <dueca/CommObjectReader.hxx>
#include <dueca/CommObjectElementReader.hxx>
#include <dueca_ns.h>
#include <map>
#include <msgpack/v3/null_visitor_decl.hpp>
#define MSGPACK_USE_BOOST
#include <msgpack.hpp>
#include <dueca/debug.h>
#include <DCOTypeIndex.hxx>
#include <msgpack/v3/object_fwd_decl.hpp>
#include <list>

DUECA_NS_START;
WEBSOCK_NS_START;

typedef msgpack::packer<std::ostream> mwriter_t;

template<class T>
void writeAny(mwriter_t& writer, const boost::any& val);

template<unsigned mxsize>
void writeAnyDstring(mwriter_t& writer, const boost::any& val)
{
  size_t l = boost::any_cast<Dstring<mxsize>>(val).size();
  writer.pack_str(l);
  writer.pack_str_body(boost::any_cast<Dstring<mxsize>>(val).c_str(), l);
}

template<>
void writeAny<char>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_char(boost::any_cast<char>(val));
}

template<>
void writeAny<uint8_t>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_uint8(boost::any_cast<uint8_t>(val));
}
template<>
void writeAny<uint16_t>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_uint16(boost::any_cast<uint16_t>(val));
}
template<>
void writeAny<uint32_t>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_uint32(boost::any_cast<uint32_t>(val));
}
template<>
void writeAny<uint64_t>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_uint64(boost::any_cast<uint64_t>(val));
}
template<>
void writeAny<int8_t>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_int8(boost::any_cast<int8_t>(val));
}
template<>
void writeAny<int16_t>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_int16(boost::any_cast<int16_t>(val));
}
template<>
void writeAny<int32_t>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_int32(boost::any_cast<int32_t>(val));
}
template<>
void writeAny<int64_t>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_int64(boost::any_cast<int64_t>(val));
}
template<>
void writeAny<bool>(mwriter_t& writer, const boost::any& val)
{
  if (boost::any_cast<bool>(val)) {
    writer.pack_true();
  }
  else {
    writer.pack_false();
  }
}
template<>
void writeAny<float>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_float(boost::any_cast<float>(val));
}
template<>
void writeAny<double>(mwriter_t& writer, const boost::any& val)
{
  writer.pack_double(boost::any_cast<double>(val));
}
template<>
void writeAny<std::string>(mwriter_t& writer, const boost::any& val)
{
   size_t l = boost::any_cast<std::string>(val).size();
  writer.pack_str(l);
  writer.pack_str_body(boost::any_cast<std::string>(val).c_str(), l);
 }

void code_value(msgpack::packer<std::ostream>& writer,
                const boost::any& val)
{
  typedef std::function<void(msgpack::packer<std::ostream>&,
                             const boost::any&)> avfunction;
  typedef std::map<typeindex_t,avfunction>
    writermap_t;
  static writermap_t wmap;
  if (wmap.size() == 0) {
    wmap[TYPEID(uint8_t)] = avfunction(writeAny<uint8_t>);
    wmap[TYPEID(uint16_t)] = avfunction(writeAny<uint16_t>);
    wmap[TYPEID(uint32_t)] = avfunction(writeAny<uint32_t>);
    wmap[TYPEID(uint64_t)] = avfunction(writeAny<uint64_t>);
    wmap[TYPEID(char)] = avfunction(writeAny<char>);
    wmap[TYPEID(int8_t)] = avfunction(writeAny<int8_t>);
    wmap[TYPEID(int16_t)] = avfunction(writeAny<int16_t>);
    wmap[TYPEID(int32_t)] = avfunction(writeAny<int32_t>);
    wmap[TYPEID(int64_t)] = avfunction(writeAny<int64_t>);
    wmap[TYPEID(bool)] = avfunction(writeAny<bool>);
    wmap[TYPEID(float)] = avfunction(writeAny<float>);
    wmap[TYPEID(double)] = avfunction(writeAny<double>);
    wmap[TYPEID(std::string)] = avfunction(writeAny<std::string>);
    wmap[TYPEID(Dstring<8>)] = avfunction(writeAnyDstring<8>);
    wmap[TYPEID(Dstring<16>)] = avfunction(writeAnyDstring<16>);
    wmap[TYPEID(Dstring<32>)] = avfunction(writeAnyDstring<32>);
    wmap[TYPEID(Dstring<64>)] = avfunction(writeAnyDstring<64>);
    wmap[TYPEID(Dstring<128>)] = avfunction(writeAnyDstring<128>);
    wmap[TYPEID(Dstring<LSSIZE>)] = avfunction(writeAnyDstring<LSSIZE>);
  }
  try {
    wmap.at(val.type())(writer, val);
  }
  catch (const boost::bad_any_cast & e) {
    /* DUECA XML.

       Failure to serialize a part of DCO object to XML due to a bad
       any_cast. */
    E_XTR("cannot serialize to msgpack, bad cast " << e.what());
  }
  catch (const std::out_of_range& e) {
    /* DUECA XML.

       Have no mapping to serialize a member of a DCO object of this datatype
       to XML. Use other datatypes in your DCO, or try to get the serialize
       expanded. */
    E_XTR("No mapping to serialize type '" << val.type().name());
  }
  catch (const std::exception &e) {
    /* DUECA XML.

       Generic failure to serialize a DCO object to XML. */
    E_XTR("Cannot serialize to msgpack " << e.what());
  }
}


void code_dco(msgpack::packer<std::ostream>& writer,
              const CommObjectReader& reader)
{
  // pack this object as a map
  writer.pack_map(reader.getNumMembers());


  for (size_t ii = 0; ii < reader.getNumMembers(); ii++) {

    // ElementReader for each elt
    ElementReader eread = reader[ii];
    size_t len = strlen(reader.getMemberName(ii));
    writer.pack_str(len);
    writer.pack_str_body(reader.getMemberName(ii), len);

    MemberArity ar = reader[ii].getArity();

    boost::any key;
    if (eread.isNested()) {
      switch(ar) {
        case Mapped: {
          writer.pack_map(eread.size());
          while (!eread.isEnd()) {
            CommObjectReader rec = eread.recurse(key);
            code_value(writer, key);
            code_dco(writer, rec);
          }
        }
          break;
        case Single: {
          CommObjectReader rec = eread.recurse(key);
          code_dco(writer, rec);
        }
          break;
        case Iterable:
        case FixedIterable: {
          writer.pack_array(eread.size());
          while (!eread.isEnd()) {
            CommObjectReader rec = eread.recurse(key);
            code_dco(writer, rec);
          }
        }
      }
    }
    else {
      boost::any value;
      switch (ar) {
        case Mapped: {
          writer.pack_map(eread.size());
          while (!eread.isEnd()) {
            eread.read(value, key);
            code_value(writer, key);
            code_value(writer, value);
          }
        }
          break;
        case Single: {
          eread.read(value, key);
          code_value(writer, value);
        }
          break;
        case Iterable:
        case FixedIterable: {
          writer.pack_array(eread.size());
          while (!eread.isEnd()) {
            eread.read(value, key);
            code_value(writer, value);
          }
        }
      }
    }
  }
}

template<class T>
void readAny(const msgpack::object &doc, boost::any& val)
{
  val = doc.as<T>();
}

template<unsigned int mxsize>
void readAnyDstring(const msgpack::object &doc, boost::any& val)
{
  val = Dstring<mxsize>(doc.as<std::string>());
}

// specialization
template<>
void readAny<dueca::smartstring>(const msgpack::object &doc, boost::any& val)
{
  val = dueca::smartstring(doc.as<std::string>());
}

boost::any decode_value(const msgpack::object& doc, typeindex_t tix)
{
  typedef std::function<void(const msgpack::object&,boost::any&)> avfunction;
  typedef std::map<typeindex_t,avfunction> writermap_t;

  static writermap_t wmap;
  if (wmap.size() == 0) {
    wmap[TYPEID(uint8_t)] = avfunction(readAny<uint8_t>);
    wmap[TYPEID(uint16_t)] = avfunction(readAny<uint16_t>);
    wmap[TYPEID(uint32_t)] = avfunction(readAny<uint32_t>);
    wmap[TYPEID(uint64_t)] = avfunction(readAny<uint64_t>);
    wmap[TYPEID(char)] = avfunction(readAny<char>);
    wmap[TYPEID(int8_t)] = avfunction(readAny<int8_t>);
    wmap[TYPEID(int16_t)] = avfunction(readAny<int16_t>);
    wmap[TYPEID(int32_t)] = avfunction(readAny<int32_t>);
    wmap[TYPEID(int64_t)] = avfunction(readAny<int64_t>);
    wmap[TYPEID(bool)] = avfunction(readAny<bool>);
    wmap[TYPEID(float)] = avfunction(readAny<float>);
    wmap[TYPEID(double)] = avfunction(readAny<double>);
    wmap[TYPEID(std::string)] = avfunction(readAny<std::string>);
    wmap[TYPEID(smartstring)] = avfunction(readAny<smartstring>);
    wmap[TYPEID(Dstring<8>)] = avfunction(readAnyDstring<8>);
    wmap[TYPEID(Dstring<16>)] = avfunction(readAnyDstring<16>);
    wmap[TYPEID(Dstring<32>)] = avfunction(readAnyDstring<32>);
    wmap[TYPEID(Dstring<64>)] = avfunction(readAnyDstring<64>);
    wmap[TYPEID(Dstring<128>)] = avfunction(readAnyDstring<128>);
    wmap[TYPEID(Dstring<LSSIZE>)] = avfunction(readAnyDstring<LSSIZE>);
  }

  boost::any val;
  try {
    auto wf = wmap.find(tix);
    if (wf != wmap.end()) {
      wf->second(doc, val);
    }
    else {
      // fallback to string? for enums
      val = doc.as<std::string>();
    }
  }
  catch (const boost::bad_any_cast &e) {
    /* DUECA websock.

       Unexpected error in converting a XML string to a DCO
       write. Cannot interpret the given variable type.
    */
    E_XTR("Cannot write msgpack value into DCO " << e.what());
    val = std::string("-- cannot decode --");
  }
  catch (const std::exception &e) {
    /* DUECA websock.

       Unexpected error in converting a XML string to a DCO write.
    */
    E_XTR("Cannot write msgpack value into DCO " << e.what());
    val = std::string("-- cannot decode --");
  }
  return val;
}

// typedef std::map<std::string, msgpack::type::variant_ref> mainmap_t;
typedef std::map<std::string, msgpack::object> mainmap_t;
typedef std::vector<msgpack::object> mainvec_t;
WEBSOCK_NS_END;
DUECA_NS_END;

namespace msgpack {
namespace adaptor {

template<>
struct as<dueca::websock::mainvec_t> {
  dueca::websock::mainvec_t operator() (const msgpack::object& obj)
  {
    if (obj.type != msgpack::type::ARRAY) { throw::msgpack::type_error(); }
    dueca::websock::mainvec_t v; v.resize(obj.via.array.size);
    for (unsigned ii = 0; ii < obj.via.array.size; ii++) {
      v[ii] = *(obj.via.array.ptr + ii);
    }
    return v;
  }
};

template<>
struct as<dueca::websock::mainmap_t> {
  dueca::websock::mainmap_t operator() (const msgpack::object& obj)
  {
    if (obj.type != msgpack::type::MAP) { throw::msgpack::type_error(); }
    dueca::websock::mainmap_t v;
    for (unsigned ii = 0; ii < obj.via.map.size; ii++) {
      auto &elt = *(obj.via.map.ptr + ii);
      assert(elt.key.type == msgpack::type::STR);
      v.emplace(std::string(elt.key.via.str.ptr, elt.key.via.str.size), elt.val);
    }
    return v;
  }
};

} // namespace adaptor
}  // namespace msgpack

DUECA_NS_START;
WEBSOCK_NS_START;

inline void decode_dco(const mainmap_t& obj, CommObjectWriter& dco)
{
  for (const auto& elt: obj) {
    try {

      ElementWriter ew = dco[elt.first.c_str()];
      if (ew.isNested()) {
        switch (ew.getArity()) {
          case Single: {
            CommObjectWriter nest = ew.recurse();
            auto args = elt.second.as<mainmap_t>();
            decode_dco(args, nest);
          }
            break;
          case Mapped: {
            auto elts = elt.second.as<mainmap_t>();
            for (const auto &e: elts) {
              boost::any key = e.first;
              CommObjectWriter nest = ew.recurse(key);
              auto args = e.second.as<mainmap_t>();
              decode_dco(args, nest);
            }
          }
            break;
          case Iterable:
          case FixedIterable: {
            auto elts = elt.second.as<mainvec_t>();
            for (const auto &e: elts) {
              CommObjectWriter nest = ew.recurse();
              auto args = e.as<mainmap_t>();
              decode_dco(args, nest);
            }
          }
        }
      }
      else {
        switch (ew.getArity()) {
          case Single: {
            ew.write(decode_value(elt.second, ew.getTypeIndex()));
          }
            break;
          case Mapped: {
            auto elts = elt.second.as<mainmap_t>();
            for (const auto &e: elts) {
              boost::any key = e.first;
              boost::any val = decode_value(e.second, ew.getTypeIndex());
              ew.write(val, key);
            }
          }
            break;
          case Iterable:
          case FixedIterable: {
            auto elts = elt.second.as<mainvec_t>();
            for (const auto &e: elts) {
              ew.write(decode_value(e, ew.getTypeIndex()));
            }
          }
        }
      }
    }
    catch (const std::exception& e) {
      E_XTR("Cannot write msgpack value into DCO " << e.what());
    }
  }
}

struct msgpackpacker
{
  msgpack::packer<std::ostream> writer;

  msgpackpacker(std::ostream& buffer) : writer(buffer) {}

  inline void StartObject(size_t n) { writer.pack_map(n); }

  inline void EndObject() {}

  inline void Key(const char *k)
  {
    writer.pack_str(strlen(k));
    writer.pack_str_body(k, strlen(k));
  }

  inline void StartArray(size_t n) { writer.pack_array(n); }

  inline void EndArray() {}

  inline void String(const char *s)
  {
    writer.pack_str(strlen(s));
    writer.pack_str_body(s, strlen(s));
  }

  inline void String(const std::string& s)
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

  inline void dco(const DCOReader &r) {
    code_dco(writer, r);
  }

  inline void EndLine() {}

  /** websockets opcode for a packet, in this case binary */
  static inline unsigned char OpCode() { return 130; }
};

struct msgpackunpacker
{
  /** Object handle, must remain alive while unpacking the object */
  msgpack::object_handle oh;

  /** Object itself */
  msgpack::object obj;

  /** Map with immediate objects */
  mainmap_t doc;

  msgpackunpacker(const std::string &s) : doc()
  {
    oh = msgpack::unpack(s.c_str(), s.size());
    obj = oh.get();
    doc = obj.as<mainmap_t>();
  }

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
};

WEBSOCK_NS_END;
DUECA_NS_END;

#include <dueca/undebug.h>

