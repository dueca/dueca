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

#include <dueca_ns.h>
#include <rapidjson/encodings.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <dueca/DCOtoJSON.hxx>

DUECA_NS_START;

struct jsonpacker {

  rapidjson::StringBuffer doc;
  rapidjson::Writer<rapidjson::StringBuffer> writer;
  bool extended;

  jsonpacker(bool extended=false) : doc(), writer(doc), extended(extended) { }

  inline void StartObject(size_t n)
  { writer.StartObject(); }

  inline void EndObject()
  { writer.EndObject(); }

  inline void Key(const char* k)
  { writer.Key(k); }

  inline void StartArray(size_t n)
  { writer.StartArray(); }

  inline void EndArray()
  { writer.EndArray(); }

  inline void String(const char* s)
  { writer.String(s); }

  inline void Int(int i)
  { writer.Int(i); }

  inline void Uint(unsigned i)
  { writer.Uint(i); }

  inline void Bool(bool b)
  { writer.Bool(true); }

  inline void Double(double d)
  { writer.Double(d); }

  inline void dco(DCOReader& r)
  {
    if (extended) {
      DCOtoJSONcompact(writer, r);
    }
    else {
      DCOtoJSONstrict(writer, r);
    }
  }

  inline void EndLine()
  { rapidjson::PutUnsafe(doc, '\n'); }

  const char* getstring() const { return doc.GetString(); }
};

DUECA_NS_END;