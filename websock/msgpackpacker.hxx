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

#include "CommObjectMemberArity.hxx"
#include <dueca/CommObjectReader.hxx>
#include <dueca/CommObjectElementReader.hxx>
#include <dueca_ns.h>
#include <msgpack.hpp>
#include <dueca/debug.h>

DUECA_NS_START;

class DCOReader;

void code_element(msgpack::packer<std::ostream>& writer,
                  const CommObjectReader& reader)
{
  writer.pack_map(reader.getNumMembers());
  for (size_t ii = 0l ii < reader.getNumMembers(); ii++) {
    ElementReader eread reader[ii];
    size_t len = strlen(reader.getMemberName(ii));
    writer.pack_str(len);
    writer.pack_str_body(reader.getMemberName(), len);

    MemberArity ar = reader[ii].getArity();
    if (ar != Single) { writer.pack_array(eread) }
  }
  if (ew.isNested()) {

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

  inline void dco(DCOReader &r) {
    writer.pack_map(r.getNumMembers());
    for (unsigned i = 0; i < r.getNumMembers(); i++) {
        auto l = strlen(r.getMemberName(i));
            writer.pack_str(l);
        writer.pack_str_body(r.getMemberName(i), l);
        switch (r.getMemberArity(i)) {
            case Single:
                code_element(writer, r[i]);
                break;
            case Iterable:
            case FixedIterable: {
                writer.pack_array(r.getMemberSize(i));
                auto er = r[i];
                for (unsigned j = 0; j < r.getMemberSize(i); j++) {
                    code_element(er);
                }
            }
                break;
            case Mapped:
                writer.pack_map(r.getMemberSize(i));
                auto er = r[i];
                for (unsigned j = 0; j < r.getMemberSize(i); j++) {
                    code_key_and_element(er);
                }
            }
        }
    }

  inline void EndLine() {}
};

struct msgpackunpacker
{
  typedef std::map<std::string, msgpack::object> mainmap_t;
  mainmap_t doc;
  msgpackunpacker(const std::string &s) : doc()
  {
    msgpack::unpacked msg;
    msgpack::unpack(&msg, s.c_str(), s.size());
    msgpack::object obj = msg.get();
    doc = obj.as<mainmap_t>();
  }

  inline DataTimeSpec getStreamTime() const
  {
    auto it = doc["tick"];
    DataTimeSpec ts;
    it.convert(&ts);
    return ts;
  }

  inline DataTimeSpec getTime() const
  {
    auto it = doc["tick"];
    TypeTickType tick;
    it.convert(&tick);
    return DataTimeSpec(tick, tick);
  }

  inline void codedToDCO(DCOWriter &wr) const
  {
    auto data = doc["data"];
    MSGPACKtoDCO(doc["data"], wr);
  }
};

#include <dueca/undebug.h>

DUECA_NS_END;