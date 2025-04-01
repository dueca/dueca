/* ------------------------------------------------------------------   */
/*      item            : JSONtoDCO.cxx
        made by         : Rene' van Paassen
        date            : 181027
        category        : body file
        description     :
        changes         : 181027 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define JSONtoDCO_cxx
#include "JSONtoDCO.hxx"
#include <DCOTypeIndex.hxx>
#include "CommObjectElementWriter.hxx"
#include <debug.h>
#include <unordered_map>
#include <dueca/Dstring.hxx>
#include <dueca/LogString.hxx>
#include <smartstring.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START;


template<class T>
void readAny(const JValue &doc, boost::any& val);

template<unsigned mxsize>
void readAnyDstring(const JValue &doc, boost::any& val)
{
  val = Dstring<mxsize>(doc.GetString());
}

template<>
void readAny<char>(const JValue &doc, boost::any& val)
{
  val = char(doc.GetString()[0]);
}

template<>
void readAny<uint8_t>(const JValue &doc, boost::any& val)
{
  val = uint8_t(doc.GetUint());
}

template<>
void readAny<uint16_t>(const JValue &doc, boost::any& val)
{
  val = uint16_t(doc.GetUint());
}

template<>
void readAny<uint32_t>(const JValue &doc, boost::any& val)
{
  val = uint32_t(doc.GetUint());
}

template<>
void readAny<uint64_t>(const JValue &doc, boost::any& val)
{
  val = uint64_t(doc.GetUint64());
}
template<>
void readAny<int8_t>(const JValue &doc, boost::any& val)
{
  val = int8_t(doc.GetInt());
}

template<>
void readAny<int16_t>(const JValue &doc, boost::any& val)
{
  val = int16_t(doc.GetInt());
}

template<>
void readAny<int32_t>(const JValue &doc, boost::any& val)
{
  val = int32_t(doc.GetInt());
}

template<>
void readAny<int64_t>(const JValue &doc, boost::any& val)
{
  val = int64_t(doc.GetInt64());
}

template<>
void readAny<float>(const JValue &doc, boost::any& val)
{
  if (!doc.IsNull()) {
    val = float(doc.GetDouble());
  }
  else {
    val = std::numeric_limits<float>::quiet_NaN();
  }
}

template<>
void readAny<double>(const JValue &doc, boost::any& val)
{
  if (!doc.IsNull()) {
    val = double(doc.GetDouble());
  }
  else {
    val = std::numeric_limits<double>::quiet_NaN();
  }
}

template<>
void readAny<std::string>(const JValue &doc, boost::any& val)
{
  val = std::string(doc.GetString());
}

template<>
void readAny<smartstring>(const JValue &doc, boost::any& val)
{
  val = smartstring(doc.GetString());
}

template<>
void readAny<bool>(const JValue &doc, boost::any& val)
{
  val = bool(doc.GetBool());
}

static void readValue(const JValue &doc, boost::any& val,
                      typeindex_t tix)
{
  typedef std::function<void(const JValue&,boost::any&)> avfunction;
  typedef std::map<typeindex_t,avfunction>
    writermap_t;

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
    wmap[TYPEID(Dstring<5>)] = avfunction(readAnyDstring<5>);
    wmap[TYPEID(Dstring<8>)] = avfunction(readAnyDstring<8>);
    wmap[TYPEID(Dstring<16>)] = avfunction(readAnyDstring<16>);
    wmap[TYPEID(Dstring<32>)] = avfunction(readAnyDstring<32>);
    wmap[TYPEID(Dstring<64>)] = avfunction(readAnyDstring<64>);
    wmap[TYPEID(Dstring<128>)] = avfunction(readAnyDstring<128>);
    wmap[TYPEID(Dstring<LSSIZE>)] = avfunction(readAnyDstring<LSSIZE>);
  }

  try {
    auto wf = wmap.find(tix);
    if (wf != wmap.end()) {
      wf->second(doc, val);
    }
    else {
      // fallback to string? for enums
      val = std::string(doc.GetString());
    }
  }
  catch (const boost::bad_any_cast &e) {
    /* DUECA JSON.

       Unexpected error in converting a JSON string to a DCO
       write. Cannot interpret the given variable type.
    */
    E_XTR("Cannot write JSON value into DCO " << e.what());
    val = std::string("-- cannot decode --");
  }
  catch (const std::exception &e) {
    /* DUECA JSON.

       Unexpected error in converting a JSON string to a DCO write.
    */
    E_XTR("Cannot write JSON value into DCO " << e.what());
    val = std::string("-- cannot decode --");
  }
}

static void readValue(const JValue &doc, ElementWriter& writer)
{
  boost::any val;
  readValue(doc, val, writer.getTypeIndex());
  writer.write(val);
}

static void writeArray(const JValue &doc, ElementWriter& eobject)
{
  if (eobject.isNested()) {
    for (JValue::ConstValueIterator it = doc.Begin();
         it != doc.End(); ++it) {
      if (it->IsObject()) {
        CommObjectWriter cw = eobject.recurse();
        JSONtoDCO(*it, cw);
      }
      else {
        /* DUECA JSON.

           Trying to write objects nested in an array, but the JSON
           array elements are not objects.
        */
        W_XTR("JSON values in " << doc.GetString() <<
              " array need to be objects");
      }
    }
  }
  else {
    for (JValue::ConstValueIterator it = doc.Begin();
         it != doc.End(); ++it) {
      if (it->IsObject() || it->IsArray()) {
        /* DUECA JSON.

           Trying to write native objects nested in an array, but the
           JSON array elements cannot be interpreted as these native
           objects.
        */
        W_XTR("JSON values in " << doc.GetString() <<
              " array need to be direct values");
      }
      else {
        readValue(*it, eobject);
      }
    }
  }
}

static void writeFixedArray(const JValue &doc, ElementWriter& eobject)
{
  try {
    writeArray(doc, eobject);
    if (!eobject.arrayIsComplete()) {
      /* DUECA JSON.

         There is not enough data in a JSON representation to
         completely fill a fixed-length array in the DCO.
      */
      W_XTR("Array not completely filled in " << doc.GetString());
    }
  }
  catch (const IndexExceeded& e) {
    /* DUECA JSON.

       There was too much array data in a JSON representation for a
       fixed-length array in the DCO.
    */
    W_XTR("array length exceeded, ignoring remaining elements in " <<
          doc.GetString());
  }
}

static void writeMap(const JValue &doc, ElementWriter& eobject)
{
  // not yet implemented...
  if (!eobject.isNested()) {
    for (JValue::ConstValueIterator it = doc.Begin();
         it != doc.End(); ++it) {
      if (it->IsObject() && it->HasMember("key") && it->HasMember("value") &&
          !(*it)["value"].IsObject() && !(*it)["value"].IsArray()) {
        boost::any key;
        readValue((*it)["key"], key, eobject.getKeyTypeIndex());
        boost::any val;
        readValue((*it)["value"], val, eobject.getTypeIndex());
        eobject.write(val, key);
      }
      else {
        /* DUECA JSON.

           Trying to write a map object in a DCO, but lacking required
           "key" and "value" members in the JSON array representation.
         */
        W_XTR("Map writing needs key and value members");
      }
    }
  }
  else {
    for (JValue::ConstValueIterator it = doc.Begin();
         it != doc.End(); ++it) {
      if (it->IsObject() && it->HasMember("key") && it->HasMember("value") &&
          (*it)["value"].IsObject()) {
        boost::any key;
        readValue((*it)["key"], key, eobject.getKeyTypeIndex());
        CommObjectWriter wval = eobject.recurse(key);
        JSONtoDCO((*it)["value"], wval);
      }
      else {
        /* DUECA JSON.

           Trying to write a map object in a DCO, but lacking required
           "key" and "value" members in the JSON array representation.
        */
        W_XTR("Map writing needs key and value objects");
      }
    }
  }
}

void JSONtoDCO(const JValue &doc, CommObjectWriter& writer)
{
  /** Iterate over all members of the JSON doc */
  for (JValue::ConstMemberIterator it = doc.MemberBegin();
       it != doc.MemberEnd(); ++it) {

    // check we have the object on the DCO
    try {
      ElementWriter eobject = writer[it->name.GetString()];

      // branch, depending on object, array or value
      if (it->value.IsObject()) {

        if (eobject.isNested() && eobject.getArity() == Single) {
          CommObjectWriter wobject = eobject.recurse();
          JSONtoDCO(it->value, wobject);
        }
        else {
          /* DUECA JSON.

             Missing an object in the JSON representation needed to
             write a nested object in a DCO.
          */
          W_XTR("JSON key \"" << it->name.GetString() <<
                "\" needs object, when writing member of class " <<
                writer.getClassname() << ", ignoring");
        }
      }

      else if (it->value.IsArray()) {

        // there should be an array of nested objects here, different
        // options:
        // - fixed size array, iterate, copy over, and check sizes match
        // - variable size array or list, clear, iterate and insert
        // - map (key and value pairs in JSON), clear, iterate and insert
        switch (eobject.getArity()) {
        case Single:
          /* DUECA JSON.

             The JSON representation holds an array, but need a single
             object for the DCO element.
           */
          W_XTR("JSON key \"" << it->name.GetString() <<
                "\" has array, but element in object of class " <<
                writer.getClassname() << " is single, ignoring" );
          break;
        case Iterable:
          writeArray(it->value, eobject);
          break;
        case Mapped:
          writeMap(it->value, eobject);
          break;
        case FixedIterable:
          writeFixedArray(it->value, eobject);
          break;
        }
      }

      else {

        if (eobject.getArity() != Single || eobject.isNested()) {
          /* DUECA JSON.

             The JSON key given needs to have a single value.
          */
          W_XTR("JSON key \"" << it->name.GetString() <<
                "\" needs single type value, when writing member of class " <<
                writer.getClassname() << ", ignoring");
        }
        else {
          readValue(it->value, eobject);
        }
      }
    }
    catch(const DataClassMemberNotFound& e) {
      /* DUECA JSON.

         Missing a key in the JSON representation for a data member in
         the DCO object.
       */
      W_XTR("JSON key \"" << it->name.GetString() <<
            "\" not found in class " <<
            writer.getClassname() << ", ignoring");
    }
  }
}

DUECA_NS_END;
