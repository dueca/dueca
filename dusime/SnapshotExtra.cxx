/* ------------------------------------------------------------------   */
/*      item            : SnapshotExtra.cxx
        made by         : Rene' van Paassen
        date            : 130104
        category        : additional header code
        description     :
        changes         : 130102 first version
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/
};

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iomanip>

namespace {

std::string decode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
        return c == '\0';
    });
}

std::string encode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

}

DUECA_NS_START;

// locally added constructor
Snapshot::Snapshot(size_t data_size, const NameSet& originator,
                   SnapCoding coding) :
  data(data_size, '\0'),
  originator(originator),
  coding(coding),
  data_size(data.size())
{
  //data.resize(data_size, '\0');
}


#define __CUSTOM_COMPATLEVEL_110
// #define __CUSTOM_DEFAULT_CONSTRUCTOR

Snapshot::Snapshot(DataWriterArraySize data_size, SnapCoding coding):
  data(data_size.size, '\0'),
  originator(),
  coding(coding),
  data_size(data.size())
{
  DOBS("default constructor Snapshot");
}

Snapshot::Snapshot(size_t data_size, SnapCoding coding):
  data(data_size, '\0'),
  originator(),
  coding(coding),
  data_size(data.size())
{
  DOBS("default constructor Snapshot");
}

Snapshot::Snapshot(const toml::value& coded)
{
  originator.name = toml::find<std::string>(coded, "origin");
  readFromString(coding, toml::find<std::string>(coded, "coding"));

  switch(coding) {
  case UnSpecified:
  case Base64:
    data = decode64(toml::find<std::string>(coded, "data"));
    break;
  case JSON:
  case XML:
    data = toml::find<std::string>(coded, "data");
    break;
  case Floats: {
    std::vector<float> result = toml::find<std::vector<float> >(coded, "data");
    data.resize(result.size() * sizeof(float));
    AmorphStore store(accessData(), getDataSize());
    for (const auto v: result) { store.packData(v); }
  }
    break;
  case Doubles: {
    std::vector<double> result =
      toml::find<std::vector<double> >(coded, "data");
    data.resize(result.size() * sizeof(double));
    AmorphStore store(accessData(), getDataSize());
    for (const auto v: result) { store.packData(v); }
  }
    break;
  case JSONFile:
  case XMLFile:
  case BinaryFile: {
    std::ifstream snfile
      (toml::find<std::string>(coded, "file").c_str(), ios::binary | ios::ate);
    data_size = snfile.tellg();
    snfile.seekg(0);
    data.resize(data_size, '\0');
    snfile.read(const_cast<char*>(data.data()), data_size);
  }
    break;
  case FloatFile: {
    std::ifstream snfile(toml::find<std::string>(coded, "file").c_str());
    std::vector<float> result;
    while (!snfile.eof() && snfile.good()) {
      float tmp; snfile >> tmp; result.push_back(tmp);
    }
    data.resize(result.size() * sizeof(float));
    AmorphStore store(accessData(), getDataSize());
    for (const auto v: result) { store.packData(v); }
  }
    break;
  case DoubleFile: {
    std::ifstream snfile(toml::find<std::string>(coded, "file").c_str());
    std::vector<double> result;
    while (!snfile.eof() && snfile.good()) {
      double tmp; snfile >> tmp; result.push_back(tmp);
    }
    data.resize(result.size() * sizeof(double));
    AmorphStore store(accessData(), getDataSize());
    for (const auto v: result) { store.packData(v); }
  }
    break;
  }
}

toml::value Snapshot::tomlCode(const std::string& fname) const
{
  toml::value result {
      {"coding", getString(coding)},
      {"origin", originator.name} };

  assert (coding < BinaryFile || fname.size() > 0);

  // write result
  switch(coding) {
  case UnSpecified:
  case Base64:
    result["data"] = encode64(data);
    break;
  case JSON:
  case XML:
    result["data"] = data;
    break;
  case Floats: {
    AmorphReStore store(accessData(), getDataSize());
    result["data"] = toml::array();
    while (!store.isExhausted()) {
      float tmp(store);
      result["data"].push_back(tmp);
    }
  }
    break;
  case Doubles: {
    AmorphReStore store(accessData(), getDataSize());
    result["data"] = toml::array();
    while (!store.isExhausted()) {
      double tmp(store);
      result["data"].push_back(tmp);
    }
  }
    break;
  case BinaryFile: {
    ofstream ofile(fname.c_str(), ios::binary);
    ofile.write(data.c_str(), data.size());
  }
    result["file"] = fname;
    break;
  case FloatFile: {
    AmorphReStore store(accessData(), getDataSize());
    ofstream ofile(fname.c_str());
    while (!store.isExhausted()) {
      float tmp(store);
      ofile << std::setprecision(8) << tmp << std::endl;
    }
    result["file"] = fname;
  }
    break;
  case DoubleFile: {
    AmorphReStore store(accessData(), getDataSize());
    ofstream ofile(fname.c_str());
    while (!store.isExhausted()) {
      double tmp(store);
      ofile << std::setprecision(15) << tmp << std::endl;
    }
    result["file"] = fname;
  }
    break;
  case JSONFile:
  case XMLFile: {
    ofstream ofile(fname.c_str());
    ofile << data;
    result["file"] = fname;
  }
    break;
  }
  return result;
}

bool Snapshot::saveExternal() const
{
  switch (coding) {
  case BinaryFile:
  case FloatFile:
  case DoubleFile:
  case JSONFile:
  case XMLFile:
    return true;
  default:
    return false;
  }
}

const char* Snapshot::fileExtension() const
{
  static const char* inco = ".inco";
  static const char* bin = ".bin";
  static const char* json = ".json";
  static const char* xml = ".xml";
  static const char* you_should_not =
    "you should not try to save this snapshot type in an external file";
  switch (coding) {
  case BinaryFile:
    return bin;
  case FloatFile:
  case DoubleFile:
    return inco;
  case JSONFile:
    return json;
  case XMLFile:
    return xml;
  default:
    return you_should_not;
  }
}

std::string Snapshot::getSample(unsigned size) const
{
  if (size < 4) return " ...";
  switch(coding) {
  case UnSpecified:
  case Base64: {
    std::string tmpdata = encode64(data);
    if (tmpdata.size() < size - 4) {
      return tmpdata;
    }
    return tmpdata.substr(0, size - 4) + std::string(" ...");
  }
  break;

  case Floats:
  case FloatFile: {
    std::stringstream res; res << "[";
    AmorphReStore store(accessData(), getDataSize());
    while (res.str().size() + 8 < size && !store.isExhausted()) {
      float tmp(store);
      res << " " << tmp << ",";
    }
    res << " ...";
    return res.str();
  }

  case Doubles:
  case DoubleFile: {
    std::stringstream res; res << "[";
    AmorphReStore store(accessData(), getDataSize());
    while (res.str().size() + 8 < size && !store.isExhausted()) {
      double tmp(store);
      res << " " << tmp << ",";
    }
    res << " ...";
    return res.str();
  }

  default:
    // ascii-based encodings
    if (data.size() + 4 < size) {
      return data;
    }
    return data.substr(0, size - 4) + std::string(" ...");
  }
}

//#define __CUSTOM_FULL_CONSTRUCTOR
/* Snapshot::Snapshot(
    const varvector<char>& data,
        const NameSet& originator) :
    data(data),
    originator(originator),
    data_size(data.size())
{
  DOBS("complete constructor Snapshot");
}
*/

#define __CUSTOM_COPY_CONSTRUCTOR
Snapshot::Snapshot(const Snapshot& other):
    data(other.data),
    originator(other.originator),
    data_size(data.size())
{
  DOBS("copy constructor Snapshot");
}

#define __CUSTOM_AMORPHRESTORE_CONSTRUCTOR
Snapshot::Snapshot(AmorphReStore& s):
  data(s),
  originator(s)
{
  // unpackiterable(s, this->data, pack_traits<varvector<char> >());
  data_size = data.size();
  DOBS("amorph constructor Snapshot");
}

#define __CUSTOM_FUNCTION_UNPACKDATA
void Snapshot::unPackData(AmorphReStore& s)
{
  DOBS("unPackData Snapshot");

  ::unPackData(s, this->data);
  ::unPackData(s, this->originator);

  //unpackiterable(s, this->data, pack_traits<varvector<char> >());
  data_size = data.size();
}

#define __CUSTOM_FUNCTION_UNPACKDATADIFF
void Snapshot::unPackDataDiff(AmorphReStore& s)
{
  DOBS("unPackDataDiff Snapshot");
  IndexRecall im;
  //checkandunpackdiffiterable(this->data, s, im,
  //                           diffpack_traits<varvector<char> >());
  checkandunpackdiffsingle(this->data, s, im);
  checkandunpackdiffsingle(this->originator, s, im);
  data_size = data.size();
}

#define __CUSTOM_OPERATOR_ASSIGN
Snapshot&
Snapshot::operator=(const Snapshot& other)
{
  DOBS("operator = Snapshot");
  if (this == &other) return *this;
  this->data = other.data;
  this->originator = other.originator;
  this->data_size = other.data_size;
  return *this;
}
