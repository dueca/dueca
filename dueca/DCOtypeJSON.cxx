/* ------------------------------------------------------------------   */
/*      item            : DCOtypeJSON.cxx
        made by         : Rene' van Paassen
        date            : 220214
        category        : body file
        description     :
        changes         : 220214 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DCOtypeJSON_cxx
#include "DCOtypeJSON.hxx"

#include "DCOTypeIndex.hxx"
#include "DataClassRegistry.hxx"
#include "CommObjectMemberAccess.hxx"
#include <dueca/DataSetConverter.hxx>

DUECA_NS_START;

void DCOtypeJSON(rapidjson::StringBuffer &doc,
                 const char* dcoclass)
{
  rapidjson::Writer<rapidjson::StringBuffer> writer(doc);
  DCOtypeJSON(writer, dcoclass);
}

static void DCOEnumOptions(rapidjson::Writer<rapidjson::StringBuffer> &writer,
			   const CommObjectReaderWriter &dco, unsigned im)
{
  auto converter = DataClassRegistry::single().getConverter(dco.getClassname());
  void *object = converter->clone(NULL);

  // reader and writer are used to find enum names
  auto eltreader = dco.getMemberAccessor(im).getReader(object);
  auto eltwriter = dco.getMemberAccessor(im).getWriter(object);

  writer.Key("enumvalues");
  writer.StartObject();

  eltwriter.setFirstValue();
  do {
    std::string value;
    long ivalue;
    eltreader.peek(value);
    eltreader.peek(ivalue);
    writer.Key(value.c_str());
    writer.Int(ivalue);
  }
  while (eltwriter.setNextValue());
  writer.EndObject();
}

static void DCOlistMembers(rapidjson::Writer<rapidjson::StringBuffer> &writer,
                           const char* dcoclass)
{
  CommObjectReaderWriter dcoinfo(dcoclass);
  DataClassRegistry_entry_type dcoentry = DataClassRegistry::single().getEntry(dcoclass);

  writer.Key("members");
  writer.StartArray();

  for (unsigned im=0; im < dcoinfo.getNumMembers(); im++) {
    CommObjectMemberAccessBasePtr dcoi =
      DataClassRegistry::single().getMemberAccessor(dcoentry, im);

    writer.StartObject();
    writer.Key("name");
    writer.String(dcoi->getName());
    writer.Key("class");
    writer.String(dcoi->getClassname());
    writer.Key("type");
    if (dcoi->getNested()) {
      writer.String("object");
      DCOlistMembers(writer, dcoi->getClassname());
    }
    else if (dcoi->isEnum()) {
      writer.String("enum");
      writer.Key("enumint");
      writer.String(dcoi->getEnumIntName());
      DCOEnumOptions(writer, dcoinfo, im);
    }
    else {
      writer.String("primitive");
    }
    switch (dcoi->getArity()) {
    case FixedIterable:
      writer.Key("size");
      writer.Uint(dcoi->getSize());
      // no break!
    case Iterable:
      writer.Key("container");
      writer.String("array");
      break;
    case Mapped:
      writer.Key("container");
      writer.String("map");
      writer.Key("key_class");
      writer.String(dcoi->getKeyClassname());
      break;
    case Single:
      break;
    }
    writer.EndObject();
  }
  writer.EndArray();
}

void DCOtypeJSON(rapidjson::Writer<rapidjson::StringBuffer> &writer,
                 const char* dcoclass)
{
  writer.StartObject();
  writer.Key("class");
  writer.String(dcoclass);
  writer.Key("type");
  writer.String("object");
  DCOlistMembers(writer, dcoclass);
  writer.EndObject();
}

DUECA_NS_END;
