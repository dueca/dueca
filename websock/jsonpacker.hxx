/* ------------------------------------------------------------------   */
/*      item            : jsonpacker.hxx
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

#include "CommObjectReader.hxx"
#include "CommObjectWriter.hxx"
#include "DataTimeSpec.hxx"
#include "JSONtoDCO.hxx"
#include "WebsockExceptions.hxx"
#include <debug.h>
#include <dueca/DCOtoJSON.hxx>
#include <dueca_ns.h>
#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

DUECA_NS_START;
WEBSOCK_NS_START;

/** Encoder that packs to json, or decodes from json */
struct jsonpacker
{

  bool extended;
  rapidjson::OStreamWrapper osw;
  rapidjson::Writer<rapidjson::OStreamWrapper> writer;

  jsonpacker(std::ostream &os, bool extended = true) :
    extended(extended), osw(os), writer(osw)
  {}

  inline void StartObject(size_t n) { writer.StartObject(); }

  inline void EndObject() { writer.EndObject(); }

  inline void Key(const char *k) { writer.Key(k); }

  inline void StartArray(size_t n) { writer.StartArray(); }

  inline void EndArray() { writer.EndArray(); }

  inline void String(const char *s) { writer.String(s); }

  inline void String(const std::string &s) { writer.String(s.c_str()); }

  inline void Int(int i) { writer.Int(i); }

  inline void Uint(unsigned i) { writer.Uint(i); }

  inline void Bool(bool b) { writer.Bool(true); }

  inline void Double(double d) { writer.Double(d); }

  inline void dco(const DCOReader &r)
  {
    if (extended) {
      DCOtoJSONcompact(writer, r);
    }
    else {
      DCOtoJSONstrict(writer, r);
    }
  }

  inline void EndLine() { rapidjson::PutUnsafe(osw, '\n'); }

  /** websockets opcode for a packet, in this case text encoded */
  static inline unsigned char OpCode() { return 129; }
};

struct jsonunpacker
{

  rapidjson::Document doc;

  /** Construct a new unpacker. */
  jsonunpacker(const std::string &s) : doc() { doc.Parse(s.c_str()); }

  inline DataTimeSpec getStreamTime() const
  {
    auto it = doc.FindMember("tick");
    if (it == doc.MemberEnd() || !it->value.IsArray() ||
        it->value.Size() != 2 || !it->value[0].IsInt()) {
        /* DUECA websockets.

         For writing data as stream (dueca::Channel::Continuous),
         the client needs to supply a "tick" member in the JSON with
         two integer values for the time tick. Check/correct the
         configuration or your external client program.
      */
      W_XTR("JSON data needs 2 elt tick");
      throw dataparseerror();
    }
    return DataTimeSpec(it->value[0].GetInt(), it->value[1].GetInt());
  }

  inline DataTimeSpec getTime() const
  {
    auto it = doc.FindMember("tick");
    if (it == doc.MemberEnd() || !it->value.IsInt()) {
      /* DUECA websockets.

        For writing data as stream (dueca::Channel::Continuous),
        the client needs to supply a "tick" member in the JSON with
        one integer value for the time tick. Check/correct the
        configuration or your external client program.
      */
      W_XTR("JSON data needs 1 elt tick");
      throw dataparseerror();
    }
      // tick needs to be a single value
    return DataTimeSpec(it->value[0].GetInt());
  }

  inline void codedToDCO(DCOWriter &wr) const
  {
    auto data = doc.FindMember("data");
    if (data == doc.MemberEnd()) {
      /* DUECA websockets.

         Error in interpreting the recurring JSON data for a
         "write-and-read" URL, it needs a member "data" with the
         to-be-written data.
      */
      W_XTR("Coded message has no member data");
      throw dataparseerror();
    }
    JSONtoDCO(doc["data"], wr);
  }
};
#include <undebug.h>

WEBSOCK_NS_END;
DUECA_NS_END;