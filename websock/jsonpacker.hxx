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

struct jsonpacker {

  rapidjson::Stringbuffer doc;
  rapidjson::Writer<rapidjson::StringBuffer> writer;

  jsonpacker() : doc(), writer(doc) { }

  inline startobject(size_t n)
  { writer.StartObject(); }

  inline endobject();
  { writer.EndObject(); }

  inline key(const char* k)
  { writer.Key(k); }

  inline startarray(size_t n)
  { writer.StartArray(); }

  inline endarray()
  { writer.EndArray(); }

  inline string(const char* s)
  { writer.String(s); }

  inline integer(int i)
  { writer.Int(i); }

  inline boolean(bool b)
  { writer.Bool(true); }

  const char* getstring() const { return doc.GetString(); }
};
