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

DUECA_NS_START;

class DCOReader;

struct msgpackcoder
{

  std::string buffer;
  msgpack::packer<std::string> writer;

  jsonpacker(bool extended = false) : buffer(), writer(buffer) {}

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

  const char *getstring() const { return buffer.c_str(); }
};

DUECA_NS_END;