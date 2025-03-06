/* ------------------------------------------------------------------   */
/*      item            : DCOtoJSON.cxx
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

#include <rapidjson/ostreamwrapper.h>
#include <stdexcept>
#define DCOtoJSON_cxx
#include "DCOtoJSON.hxx"

#include <DCOTypeIndex.hxx>
#include <iostream>
#include <unordered_map>
//#include <function>
#include <boost/any.hpp>
#include <dueca/CommObjectReader.hxx>
#include <dueca/CommObjectElementReader.hxx>
#include <dueca/Dstring.hxx>
#include <dueca/LogString.hxx>
#include <smartstring.hxx>
#include <debug.h>
#include <functional>
#include <ActivityContext.hxx>
#include <cmath>

namespace json = rapidjson;

DUECA_NS_START;

template<typename WRITER, class T>
void writeAny(WRITER& writer, const boost::any& val);

template<typename WRITER, typename F>
void writefloatflex(WRITER& writer,
                    const boost::any& val) {
  F v = boost::any_cast<F>(val);
  writer.Double(v);
};

template<typename WRITER, typename F>
void writefloatstrict(WRITER& writer,
                      const boost::any &val) {
  F v = boost::any_cast<F>(val);
  if (std::isfinite(v)) {
    writer.Double(v);
  }
  else if (std::isnan(v)) {
    writer.Null();
  }
  else if (std::signbit(v)) {
    writer.Double(-1e200);
  }
  else {
    writer.Double(1e200);
  }
};

template<typename WRITER, unsigned mxsize>
void writeAnyDstring(WRITER& writer,
                     const boost::any& val)
{
  writer.String(boost::any_cast<Dstring<mxsize> >(val).c_str(),
                boost::any_cast<Dstring<mxsize> >(val).size(), true);
}

template<>
void writeAny<json::Writer<json::StringBuffer>,char>
(json::Writer<json::StringBuffer>& writer, const boost::any& val)
{
  char tmp[2] = { boost::any_cast<char>(val), '\0' };
  writer.String(tmp, 1, true);
}

template<>
void writeAny<json::Writer<json::OStreamWrapper>,char>
(json::Writer<json::OStreamWrapper>& writer, const boost::any& val)
{
  char tmp[2] = { boost::any_cast<char>(val), '\0' };
  writer.String(tmp, 1, true);
}

template<>
void writeAny<json::Writer<json::StringBuffer>,uint8_t>
(json::Writer<json::StringBuffer>& writer,
                       const boost::any& val)
{
  writer.Uint(boost::any_cast<uint8_t>(val));
}
template<>
void writeAny<json::Writer<json::OStreamWrapper>,uint8_t>
(json::Writer<json::OStreamWrapper>& writer,
                       const boost::any& val)
{
  writer.Uint(boost::any_cast<uint8_t>(val));
}

template<>
void writeAny<json::Writer<json::StringBuffer>,uint16_t>
(json::Writer<json::StringBuffer>& writer,
                        const boost::any& val)
{
  writer.Uint(boost::any_cast<uint16_t>(val));
}
template<>
void writeAny<json::Writer<json::OStreamWrapper>,uint16_t>
(json::Writer<json::OStreamWrapper>& writer,
                        const boost::any& val)
{
  writer.Uint(boost::any_cast<uint16_t>(val));
}

template<>
void writeAny<json::Writer<json::StringBuffer>,uint32_t>
(json::Writer<json::StringBuffer>& writer,
                        const boost::any& val)
{
  writer.Uint(boost::any_cast<uint32_t>(val));
}
template<>
void writeAny<json::Writer<json::OStreamWrapper>,uint32_t>
(json::Writer<json::OStreamWrapper>& writer,
                        const boost::any& val)
{
  writer.Uint(boost::any_cast<uint32_t>(val));
}

template<>
void writeAny<json::Writer<json::StringBuffer>,uint64_t>
(json::Writer<json::StringBuffer>& writer,
                        const boost::any& val)
{
  writer.Uint64(boost::any_cast<uint64_t>(val));
}
template<>
void writeAny<json::Writer<json::OStreamWrapper>,uint64_t>(json::Writer<json::OStreamWrapper>& writer,
                        const boost::any& val)
{
  writer.Uint64(boost::any_cast<uint64_t>(val));
}

template<>
void writeAny<json::Writer<json::StringBuffer>,int8_t>(json::Writer<json::StringBuffer>& writer,
                      const boost::any& val)
{
  writer.Int(boost::any_cast<int8_t>(val));
}
template<>
void writeAny<json::Writer<json::OStreamWrapper>,int8_t>(json::Writer<json::OStreamWrapper>& writer,
                      const boost::any& val)
{
  writer.Int(boost::any_cast<int8_t>(val));
}

template<>
void writeAny<json::Writer<json::StringBuffer>,int16_t>(json::Writer<json::StringBuffer>& writer,
                       const boost::any& val)
{
  writer.Int(boost::any_cast<int16_t>(val));
}
template<>
void writeAny<json::Writer<json::OStreamWrapper>,int16_t>(json::Writer<json::OStreamWrapper>& writer,
                       const boost::any& val)
{
  writer.Int(boost::any_cast<int16_t>(val));
}

template<>
void writeAny<json::Writer<json::StringBuffer>,int32_t>(json::Writer<json::StringBuffer>& writer,
                       const boost::any& val)
{
  writer.Int(boost::any_cast<int32_t>(val));
}
template<>
void writeAny<json::Writer<json::OStreamWrapper>,int32_t>(json::Writer<json::OStreamWrapper>& writer,
                       const boost::any& val)
{
  writer.Int(boost::any_cast<int32_t>(val));
}

template<>
void writeAny<json::Writer<json::StringBuffer>,int64_t>(json::Writer<json::StringBuffer>& writer,
                       const boost::any& val)
{
  writer.Int64(boost::any_cast<int64_t>(val));
}
template<>
void writeAny<json::Writer<json::OStreamWrapper>,int64_t>(json::Writer<json::OStreamWrapper>& writer,
                       const boost::any& val)
{
  writer.Int64(boost::any_cast<int64_t>(val));
}

template<>
void writeAny<json::Writer<json::StringBuffer>,bool>(json::Writer<json::StringBuffer>& writer,
                    const boost::any& val)
{
  writer.Bool(boost::any_cast<bool>(val));
}

template<>
void writeAny<json::Writer<json::OStreamWrapper>,bool>(json::Writer<json::OStreamWrapper>& writer,
                    const boost::any& val)
{
  writer.Bool(boost::any_cast<bool>(val));
}


template<typename WRITER, typename F,
         void (*FN)(WRITER&, const F&)>
void writeAnyFloat(WRITER& writer,
                   const boost::any& val)
{
  F v = boost::any_cast<F>(val);
  FN(writer, v);
}

template<>
void writeAny<json::Writer<json::StringBuffer>,ActivityContext>(json::Writer<json::StringBuffer>& writer,
                               const boost::any& val)
{
  std::stringstream o;
  boost::any_cast<ActivityContext>(val).print(o);
  writer.String(o.str().c_str());
}

template<>
void writeAny<json::Writer<json::OStreamWrapper>,ActivityContext>(json::Writer<json::OStreamWrapper>& writer,
                               const boost::any& val)
{
  std::stringstream o;
  boost::any_cast<ActivityContext>(val).print(o);
  writer.String(o.str().c_str());
}

template<>
void writeAny<json::Writer<json::StringBuffer>,std::string>
(json::Writer<json::StringBuffer>& writer,
                           const boost::any& val)
{
  writer.String(boost::any_cast<std::string>(val).data(),
                boost::any_cast<std::string>(val).size());
}
template<>

void writeAny<json::Writer<json::OStreamWrapper>,std::string>
(json::Writer<json::OStreamWrapper>& writer,
                           const boost::any& val)
{
  writer.String(boost::any_cast<std::string>(val).data(),
                boost::any_cast<std::string>(val).size());
}

template<>
void writeAny<json::Writer<json::StringBuffer>,smartstring>
(json::Writer<json::StringBuffer>& writer, const boost::any& val)
{
  writer.String(boost::any_cast<smartstring>(val).data(),
                boost::any_cast<smartstring>(val).size());
}

template<>
void writeAny<json::Writer<json::OStreamWrapper>,smartstring>
(json::Writer<json::OStreamWrapper>& writer, const boost::any& val)
{
  writer.String(boost::any_cast<smartstring>(val).data(),
                boost::any_cast<smartstring>(val).size());
}

// last resort, generic is not OK
#if 0
template<class T>
static void writeAny(WRITER& writer,
                     const boost::any& val)
{
  std::cerr << "not implemented" << std::endl;
}
#endif


template<typename WRITER,
        void (*Ff)(WRITER&, const boost::any&),
         void (*Fd)(WRITER&, const boost::any&)>
void writeAnyValue(WRITER& writer,
                   const boost::any& val)
{
  typedef std::function<void(WRITER&,
                           const boost::any&)> avfunction;
  typedef std::map<typeindex_t,avfunction>
    writermap_t;

  static writermap_t wmap;
  if (wmap.size() == 0) {
    wmap[TYPEID(uint8_t)] = avfunction(writeAny<WRITER,uint8_t>);
    wmap[TYPEID(uint16_t)] = avfunction(writeAny<WRITER,uint16_t>);
    wmap[TYPEID(uint32_t)] = avfunction(writeAny<WRITER,uint32_t>);
    wmap[TYPEID(uint64_t)] = avfunction(writeAny<WRITER,uint64_t>);
    wmap[TYPEID(char)] = avfunction(writeAny<WRITER,char>);
    wmap[TYPEID(int8_t)] = avfunction(writeAny<WRITER,int8_t>);
    wmap[TYPEID(int16_t)] = avfunction(writeAny<WRITER,int16_t>);
    wmap[TYPEID(int32_t)] = avfunction(writeAny<WRITER,int32_t>);
    wmap[TYPEID(int64_t)] = avfunction(writeAny<WRITER,int64_t>);
    wmap[TYPEID(bool)] = avfunction(writeAny<WRITER,bool>);
    //wmap[TYPEID(float)] = avfunction(writeAny<float>);
    //wmap[TYPEID(double)] = avfunction(writeAny<double>);
    wmap[TYPEID(float)] = avfunction(Ff);
    wmap[TYPEID(double)] = avfunction(Fd);
    wmap[TYPEID(std::string)] = avfunction(writeAny<WRITER,std::string>);
    wmap[TYPEID(smartstring)] = avfunction(writeAny<WRITER,smartstring>);
    wmap[TYPEID(Dstring<5>)] = avfunction(writeAnyDstring<WRITER,5>);
    wmap[TYPEID(Dstring<8>)] = avfunction(writeAnyDstring<WRITER,8>);
    wmap[TYPEID(Dstring<16>)] = avfunction(writeAnyDstring<WRITER,16>);
    wmap[TYPEID(Dstring<32>)] = avfunction(writeAnyDstring<WRITER,32>);
    wmap[TYPEID(Dstring<64>)] = avfunction(writeAnyDstring<WRITER,64>);
    wmap[TYPEID(Dstring<128>)] = avfunction(writeAnyDstring<WRITER,128>);
    wmap[TYPEID(Dstring<LSSIZE>)] = avfunction(writeAnyDstring<WRITER,LSSIZE>);
    wmap[TYPEID(dueca::ActivityContext)] =
      avfunction(writeAny<WRITER,dueca::ActivityContext>);
  }
  try {
    wmap.at(val.type())(writer, val);
  }
  catch (const boost::bad_any_cast & e) {
    /* DUECA JSON.

       Failure to serialize a DCO object member to JSON due to a bad
       any_cast. */
    E_XTR("cannot serialize to JSON, bad cast " << e.what());
    writer.Null();
  }
  catch (const std::out_of_range& e) {
    /* DUECA JSON.

       Have no mapping to serialize a DCO object member of this datatype
       to JSON. Use other datatypes in your DCO, or try to get
       the serialize expanded. */
    E_XTR("No mapping to serialize type '" << val.type().name());
    writer.Null();
  }
  catch (const std::exception &e) {
    /* DUECA JSON.

       Generic failure to serialize a part of a DCO object to JSON. */
    E_XTR("Cannot serialize to JSON " << e.what());
    writer.Null();
  }
}

template<typename WRITER,
         void (*Ff)(WRITER&,
                    const boost::any&),
         void (*Fd)(WRITER&,
                    const boost::any&)>
static void DCOtoJSON(WRITER& writer,
                      const CommObjectReader& reader)
{
  writer.StartObject();
  for (size_t ii = 0; ii < reader.getNumMembers(); ii++) {
    ElementReader eread = reader[ii];

    assert(reader.getMemberName(ii) != NULL);

    // first write the key
    writer.Key(reader.getMemberName(ii));

    // check arity; do we need to start an array?
    MemberArity ar = reader[ii].getArity();
    if (ar != Single) { writer.StartArray(); }

    // nested values, recursively call DCOtoJSON with a new reader
    if (eread.isNested()) {
      while(!eread.isEnd()) {

        // Create a reader on the value
        boost::any key;
        CommObjectReader rec = eread.recurse(key);

        if (ar == Mapped) {
          // for mapped objects,
          writer.StartObject();
          writer.Key("key");
          writeAnyValue<WRITER,Ff,Fd>(writer, key);
          writer.Key("value");

          // recursively call
          DCOtoJSON<WRITER,Ff,Fd>(writer, rec);
          writer.EndObject();
        }
        else {
          DCOtoJSON<WRITER,Ff,Fd>(writer, rec);
        }
      }
    }
    else {

      while(!eread.isEnd()) {
        boost::any key, value;
        eread.read(value, key);
        if (ar == Mapped) {
          // for mapped objects,
          writer.StartObject();
          writer.Key("key");
          writeAnyValue<WRITER,Ff,Fd>(writer, key);
          writer.Key("value");
          writeAnyValue<WRITER,Ff,Fd>(writer, value);
          writer.EndObject();
        }
        else {
          writeAnyValue<WRITER,Ff,Fd>(writer, value);
        }
      }
    }

    if (ar != Single) { writer.EndArray(); }
  }
  writer.EndObject();
}


void DCOtoJSONcompact(json::StringBuffer &doc,
                      const char* dcoclass, const void* object)
{
  CommObjectReader reader(dcoclass, object);
  json::Writer<json::StringBuffer> writer(doc);
  DCOtoJSON<json::Writer<json::StringBuffer>, writefloatflex<json::Writer<json::StringBuffer>,float>, writefloatflex<json::Writer<json::StringBuffer>,double> >(writer, reader);
}


void DCOtoJSONcompact(rapidjson::Writer<rapidjson::StringBuffer> &writer,
                      const CommObjectReader& reader)
{
  DCOtoJSON<json::Writer<json::StringBuffer>, writefloatflex<json::Writer<json::StringBuffer>,float>, writefloatflex<json::Writer<json::StringBuffer>,double> >(writer, reader);
}

void DCOtoJSONcompact(rapidjson::Writer<rapidjson::OStreamWrapper> &writer,
                      const CommObjectReader& reader)
{
  DCOtoJSON<json::Writer<json::OStreamWrapper>, writefloatflex<json::Writer<rapidjson::OStreamWrapper>,float>, writefloatflex<json::Writer<rapidjson::OStreamWrapper>,double> >(writer, reader);
}

void DCOtoJSONstrict(json::StringBuffer &doc,
                     const char* dcoclass, const void* object)
{
  CommObjectReader reader(dcoclass, object);
  json::Writer<json::StringBuffer> writer(doc);
  DCOtoJSON<json::Writer<json::StringBuffer>, writefloatstrict<json::Writer<json::StringBuffer>,float>, writefloatstrict<json::Writer<json::StringBuffer>,double> >(writer, reader);
}


void DCOtoJSONstrict(rapidjson::Writer<rapidjson::StringBuffer> &writer,
                     const CommObjectReader& reader)
{
  DCOtoJSON<json::Writer<json::StringBuffer>, writefloatstrict<json::Writer<json::StringBuffer>,float>, writefloatstrict<json::Writer<json::StringBuffer>,double> >(writer, reader);
}

void DCOtoJSONstrict(rapidjson::Writer<rapidjson::OStreamWrapper> &writer,
                     const CommObjectReader& reader)
{
  DCOtoJSON<json::Writer<json::OStreamWrapper>, writefloatstrict<json::Writer<rapidjson::OStreamWrapper>,float>, writefloatstrict<json::Writer<json::OStreamWrapper>,double> >(writer, reader);
}

DUECA_NS_END;
