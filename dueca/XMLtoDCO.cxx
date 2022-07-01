/* ------------------------------------------------------------------   */
/*      item            : XMLtoDCO.cxx
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

#define XMLtoDCO_cxx
#include "XMLtoDCO.hxx"
#include <DCOTypeIndex.hxx>
#include "CommObjectElementWriter.hxx"
#include <debug.h>
#include <unordered_map>
#include <dueca/Dstring.hxx>
#include <dueca/LogString.hxx>
#include "smartstring.hxx"
#define DEBPRINTLEVEL -1
#include <debprint.h>


DUECA_NS_START;

struct xmldecodeexception: public std::exception
{
  const char* reason;
  xmldecodeexception(const char* re) : reason(re) {}
  const char* what() const noexcept { return reason; }
};

template<class T>
void readAny(const pugi::xml_node &doc, boost::any& val);

template<unsigned mxsize>
void readAnyDstring(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting dstring from " << doc.child_value());
  val = Dstring<mxsize>(doc.child_value());
}

template<>
void readAny<char>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting char from " << doc.child_value());
  val = char(doc.child_value()[0]);
}

template<>
void readAny<uint8_t>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting uint8 from " << doc.child_value());
  val = boost::lexical_cast<uint8_t>(doc.child_value());
}

template<>
void readAny<uint16_t>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting uint16 from " << doc.child_value());
  val = boost::lexical_cast<uint16_t>(doc.child_value());
}

template<>
void readAny<uint32_t>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting uint32 from " << doc.child_value());
  val = boost::lexical_cast<uint32_t>(doc.child_value());
}

template<>
void readAny<uint64_t>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting uint64 from " << doc.child_value());
  val = boost::lexical_cast<uint64_t>(doc.child_value());
}
template<>
void readAny<int8_t>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting int8 from " << doc.child_value());
  val = boost::lexical_cast<int8_t>(doc.child_value());
}

template<>
void readAny<int16_t>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting int16 from " << doc.child_value());
  val = boost::lexical_cast<int16_t>(doc.child_value());
}

template<>
void readAny<int32_t>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting int32 from " << doc.child_value());
  val = boost::lexical_cast<int32_t>(doc.child_value());
}

template<>
void readAny<int64_t>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting int64 from " << doc.child_value());
  val = boost::lexical_cast<int64_t>(doc.child_value());
}

template<>
void readAny<float>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting float from " << doc.child_value());
  val = boost::lexical_cast<float>(doc.child_value());
}

template<>
void readAny<double>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting double from " << doc.child_value());
  val = boost::lexical_cast<double>(doc.child_value());
}

template<>
void readAny<std::string>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting string from " << doc.child_value());
  val = std::string(doc.child_value());
}

template<>
void readAny<smartstring>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting string from " << doc.child_value());
  val = smartstring(doc.child_value());
}

template<>
void readAny<bool>(const pugi::xml_node &doc, boost::any& val)
{
  DEB("Getting bool from " << doc.child_value());
  val = boost::lexical_cast<bool>(doc.child_value());
}

static void convertValue(pugi::xml_node& doc, boost::any& val,
                      typeindex_t tix)
{
  typedef std::function<void(const pugi::xml_node&,boost::any&)> avfunction;
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
      DEB("decoding value");
      wf->second(doc, val);
    }
    else {
      // fallback to string? for enums
      DEB("Fallback to string");
      val = std::string(doc.child_value());
    }
  }
  catch (const boost::bad_any_cast &e) {
    /* DUECA XML.

       Unexpected error in converting a XML string to a DCO
       write. Cannot interpret the given variable type.
    */
    E_XTR("Cannot write XML value into DCO " << e.what());
    val = std::string("-- cannot decode --");
  }
  catch (const std::exception &e) {
    /* DUECA XML.

       Unexpected error in converting a XML string to a DCO write.
    */
    E_XTR("Cannot write XML value into DCO " << e.what());
    val = std::string("-- cannot decode --");
  }
}

static void convertValue(pugi::xml_node &value, ElementWriter& writer)
{
  boost::any val;
  convertValue(value, val, writer.getTypeIndex());
  writer.write(val);
}

static void convertArrayObject(pugi::xml_node &object, ElementWriter& eobject)
{
  for (; object; object = object.next_sibling("object")) {
    DEB("Object from array");
    CommObjectWriter cw = eobject.recurse();
    XMLtoDCO(object, cw);
  }
}

static void convertArray(pugi::xml_node &value, ElementWriter& eobject)
{
  for (; value; value = value.next_sibling("object")) {
    DEB("Value from array");
    convertValue(value, eobject);
  }
}

static void convertFixedArray(pugi::xml_node &value, ElementWriter& eobject)
{
  try {
    convertArray(value, eobject);
    if (!eobject.arrayIsComplete()) {
      /* DUECA XML.

         There is not enough data in a XML representation to
         completely fill a fixed-length array in the DCO.
      */
      W_XTR("Array not completely filled in " << value.child_value());
    }
  }
  catch (const IndexExceeded& e) {
    /* DUECA XML.

       There was too much array data in a XML representation for a
       fixed-length array in the DCO.
    */
    W_XTR("array length exceeded, ignoring remaining elements in " <<
          value.parent().name());
  }
}

static void convertFixedArrayObject(pugi::xml_node &value, ElementWriter& eobject)
{
  try {
    convertArrayObject(value, eobject);
    if (!eobject.arrayIsComplete()) {
      /* DUECA XML.

         There is not enough data in a XML representation to
         completely fill a fixed-length array in the DCO.
      */
      W_XTR("Array not completely filled in " << value.parent().name());
    }
  }
  catch (const IndexExceeded& e) {
    /* DUECA XML.

       There was too much array data in a XML representation for a
       fixed-length array in the DCO.
    */
    W_XTR("array length exceeded, ignoring remaining elements in " <<
          value.parent().name());
  }
}

static void convertMapObjects(pugi::xml_node &pair,
                              ElementWriter& eobject)
{
  for ( ; pair; pair = pair.next_sibling("pair")) {
    pugi::xml_node object = pair.child("object");
    pugi::xml_node key = pair.child("key");
    if (key && object) {
      boost::any keyval;
      DEB("Recursing for key " << key.child_value());
      convertValue(key, keyval, eobject.getKeyTypeIndex());
      CommObjectWriter wval = eobject.recurse(key);
      XMLtoDCO(object, wval);
    }
    else {
      /* DUECA XML.

	 When trying to decode a "std::map" of objects, or simular
	 member, either the "key" or "object" tag is missing. Please
	 inspect/correct your XML.
      */
      W_XTR("Missing key or object in pair");
    }
  }
}

static void convertMap(pugi::xml_node &pair,
                     ElementWriter& eobject)
{
  for ( ; pair; pair = pair.next_sibling("pair")) {
    pugi::xml_node value = pair.child("value");
    pugi::xml_node key = pair.child("key");
    if (key && value) {
      boost::any keyval, valval;
      DEB("Value for key " << key.child_value());
      convertValue(key, keyval, eobject.getKeyTypeIndex());
      convertValue(value, valval, eobject.getTypeIndex());
      eobject.write(valval, keyval);
    }
    else {
      /* DUECA XML.

	 When trying to decode a "std::map" of values, or simular
	 member, either the "key" or "value" tag is missing. Please
	 inspect/correct your XML.
      */
       W_XTR("Missing key or value in pair");
    }
  }
}


void XMLtoDCO(const pugi::xml_node& object, CommObjectWriter& writer)
{
  /* Check this is the correct node type */
  pugi::xml_attribute classname = object.attribute("class");
  if (strcmp(classname.value(),writer.getClassname())) {
    throw xmldecodeexception("dataclass mismatch");
  }
  if (strcmp(object.name(), "object")) {
    throw xmldecodeexception("incorrect node type, expected object");
  }

  /* Run through all the members. */
  for (pugi::xml_node member = object.child("member"); member;
       member = member.next_sibling("member")) {

    // check we have the object on the DCO
    try {
      ElementWriter eobject = writer[member.attribute("name").value()];
      DEB("Member " << member.attribute("name").value());

      // branch, depending on object, array or value
      pugi::xml_node child = member.child("object");
      if (child && eobject.isNested()) {

        switch(eobject.getArity()) {
        case Single: {
          CommObjectWriter wobject = eobject.recurse();
          XMLtoDCO(child, wobject);
        }
          break;
        case Iterable:
          convertArrayObject(child, eobject);
          break;
        case Mapped:
	  /* DUECA XML.

	     When trying to decode a "std::map" or simular member,
	     zero or multiple "<pair>" tags with keys and data are
	     expected. Found an <object> tag instead.
	  */
          W_XTR("XML data mismatch, member " << member.attribute("name")
                << " expects value pairs");
          break;
        case FixedIterable:
          convertFixedArrayObject(child, eobject);
          break;
        }
      }
      else if ((child = member.child("value")) && !eobject.isNested()) {
        switch(eobject.getArity()) {
        case Single:
          convertValue(child, eobject);
          break;
        case Iterable:
          convertArray(child, eobject);
          break;
        case Mapped:
	  /* DUECA XML.

	     When trying to decode a `std::map` or simular member,
	     zero or multiple `pair` tags with keys and data are
	     expected. Found a `value` tag instead.
	  */
          W_XTR("XML data mismatch, member " << member.attribute("name")
                << " expects object pairs");
          break;
        case FixedIterable:
          convertFixedArray(child, eobject);
          break;
        }
      }
      else if ((child = member.child("pair")) && child.child("object") &&
               eobject.isNested()) {
        convertMapObjects(child, eobject);
      }
      else if ((child = member.child("pair")) && child.child("value") &&
               !eobject.isNested()) {
        convertMap(child, eobject);
      }
      else if (eobject.getArity() == Iterable ||
               eobject.getArity() == Mapped) {
        // empty object, also OK
      }
      else {
	  /* DUECA XML.

	     The XML data for a specific member is not coded in the
	     right format. Note that nested DCO objects are coded with
	     `object` tags, values are coded with `value` tags, and
	     mapped objects are coded with `pair` tags, with key and
	     value or object pairs.
	  */
        W_XTR("XML data mismatch, member " << member.attribute("name"));
      }
    }
    catch(const DataClassMemberNotFound& e) {
      /* DUECA XML.

         Missing a key in the XML representation for a data member in
         the DCO object.
       */
      W_XTR("XML key " << member.attribute("name") <<
            " not found in class " <<
            writer.getClassname() << ", ignoring");
    }
  }
}

DUECA_NS_END;
