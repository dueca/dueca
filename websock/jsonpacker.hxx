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

#include <rapidjson/encodings.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>

struct jsonpacker {

  rapidjson::StringBuffer doc;
  rapidjson::Writer<rapidjson::StringBuffer> writer;
  bool extended;

  jsonpacker(bool extended) : doc(), writer(doc), extended(extended) { }

  inline void startobject(size_t n)
  { writer.StartObject(); }

  inline void endobject();
  { writer.EndObject(); }

  inline void key(const char* k)
  { writer.Key(k); }

  inline void startarray(size_t n)
  { writer.StartArray(); }

  inline void endarray()
  { writer.EndArray(); }

  inline void string(const char* s)
  { writer.String(s); }

  inline void integer(int i)
  { writer.Int(i); }

  inline void uinteger(unsigned i)
  { writer.Uint(i); }

  inline void boolean(bool b)
  { writer.Bool(true); }

  inline void _double(double d)
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

  inline void endline()
  { rapidjson::PutUnsafe(doc, '\n'); }

  const char* getstring() const { return doc.GetString(); }
};
