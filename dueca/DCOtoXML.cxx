/* ------------------------------------------------------------------   */
/*      item            : DCOtoXML.cxx
        made by         : Rene' van Paassen
        date            : 180518
        category        : body file
        description     :
        changes         : 180518 first version
        language        : C++
        copyright       : (c) 18 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DCOtoXML_cxx
#include "DCOtoXML.hxx"

#include <DCOTypeIndex.hxx>
#include <iostream>
#include <unordered_map>
//#include <function>
#include <boost/any.hpp>
#include <dueca/CommObjectReader.hxx>
#include <dueca/CommObjectElementReader.hxx>
#include <dueca/Dstring.hxx>
#include <dueca/LogString.hxx>
#include <debug.h>
#include <functional>
#include <ActivityContext.hxx>
#include <cmath>

DUECA_NS_START;

template<class T>
void writeAny(pugi::xml_node& writer,
              const boost::any& val);

template<unsigned mxsize>
void writeAnyDstring(pugi::xml_node& writer,
                     const boost::any& val)
{
  writer.text().set(boost::any_cast<Dstring<mxsize> >(val).c_str());
}

template<>
void writeAny<char>(pugi::xml_node& writer,
                    const boost::any& val)
{
  char tmp[2] = { boost::any_cast<char>(val), '\0' };
  writer.text().set(tmp);
}


template<>
void writeAny<uint8_t>(pugi::xml_node& writer,
                       const boost::any& val)
{
  writer.text().set(boost::any_cast<uint8_t>(val));
}

template<>
void writeAny<uint16_t>(pugi::xml_node& writer,
                        const boost::any& val)
{
  writer.text().set(boost::any_cast<uint16_t>(val));
}

template<>
void writeAny<uint32_t>(pugi::xml_node& writer,
                        const boost::any& val)
{
  writer.text().set(boost::any_cast<uint32_t>(val));
}

template<>
void writeAny<uint64_t>(pugi::xml_node& writer,
                        const boost::any& val)
{
  writer.text().set(boost::any_cast<uint64_t>(val));
}

template<>
void writeAny<int8_t>(pugi::xml_node& writer,
                      const boost::any& val)
{
  writer.text().set(boost::any_cast<int8_t>(val));
}

template<>
void writeAny<int16_t>(pugi::xml_node& writer,
                       const boost::any& val)
{
  writer.text().set(boost::any_cast<int16_t>(val));
}

template<>
void writeAny<int32_t>(pugi::xml_node& writer,
                       const boost::any& val)
{
  writer.text().set(boost::any_cast<int32_t>(val));
}

template<>
void writeAny<int64_t>(pugi::xml_node& writer,
                       const boost::any& val)
{
  writer.text().set(boost::any_cast<int64_t>(val));
}

template<>
void writeAny<bool>(pugi::xml_node& writer,
                    const boost::any& val)
{
  writer.text().set(boost::any_cast<bool>(val));
}

template<>
void writeAny<float>(pugi::xml_node& writer,
                    const boost::any& val)
{
  writer.text().set(boost::any_cast<float>(val));
}

template<>
void writeAny<double>(pugi::xml_node& writer,
                      const boost::any& val)
{
  writer.text().set(boost::any_cast<double>(val));
}

template<>
void writeAny<std::string>(pugi::xml_node& writer,
                           const boost::any& val)
{
  writer.text().set(boost::any_cast<std::string>(val).c_str());
}

// last resort, generic is not OK
#if 0
template<class T>
static void writeAny(pugi::xml_node& writer,
                     const boost::any& val)
{
  std::cerr << "not implemented" << std::endl;
}
#endif


void writeAnyValue(pugi::xml_node& writer,
                   const boost::any& val)
{
  typedef std::function<void(pugi::xml_node&,
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
    E_XTR("cannot serialize to XML, bad cast " << e.what());
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
    E_XTR("Cannot serialize to XML " << e.what());
  }
}

void DCOtoXML(pugi::xml_node &doc,
              const char* dcoclass, const void* object, bool meta)
{
  CommObjectReader reader(dcoclass, object);
  DCOtoXML(doc, reader, meta);
}

void DCOtoXML(pugi::xml_node &writer,
              CommObjectReader& reader,
              bool meta)
{
  pugi::xml_node object = writer.append_child("object");
  object.append_attribute("class") = reader.getClassname();

  for (size_t ii = 0; ii < reader.getNumMembers(); ii++) {
    ElementReader eread = reader[ii];

    assert(reader.getMemberName(ii) != NULL);
    pugi::xml_node member = object.append_child("member");
    member.append_attribute("name") = reader.getMemberName(ii);

    // check arity; do we need to start an array?
    MemberArity ar = reader[ii].getArity();

    // nested values, recursively call DCOtoXML with a new reader
    if (eread.isNested()) {
      while(!eread.isEnd()) {

        // Create a reader on the value
        boost::any key;
        CommObjectReader rec = eread.recurse(key);

        if (ar == Mapped) {

          pugi::xml_node pair = member.append_child("pair");
          pugi::xml_node keynode = pair.append_child("key");
          if (meta) {
            keynode.append_attribute("type") = reader.getMemberKeyClass(ii);
          }
          writeAnyValue(keynode, key);
          DCOtoXML(pair, rec);
        }
        else {
          DCOtoXML(member, rec);
        }
      }
    }
    else {
      while(!eread.isEnd()) {
        boost::any key, value;
        eread.read(value, key);
        if (ar == Mapped) {

          pugi::xml_node pair = member.append_child("pair");
          pugi::xml_node keynode = pair.append_child("key");
          pugi::xml_node valuenode = pair.append_child("value");
          if (meta) {
            keynode.append_attribute("type") = reader.getMemberKeyClass(ii);
            valuenode.append_attribute("type") = reader.getMemberClass(ii);
          }
          writeAnyValue(valuenode, value);
        }
        else {
          pugi::xml_node valuenode = member.append_child("value");
          if (meta) {
            valuenode.append_attribute("type") = reader.getMemberClass(ii);
          }
          writeAnyValue(valuenode, value);
        }
      }
    }
  }
}



DUECA_NS_END;
