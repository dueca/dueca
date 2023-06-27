/* ------------------------------------------------------------------   */
/*      item            : CommObjectTraits.cxx
        made by         : Rene' van Paassen
        date            : 131220
        category        : body file
        description     :
        changes         : 131220 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define CommObjectTraits_cxx
#include "CommObjectTraits.hxx"
#include "smartstring.hxx"

DUECA_NS_START;

template <>
const char* getclassname<double>() { return "double"; }
template <>
const char* getclassname<float>() { return "float"; }
template <>
const char* getclassname<int8_t>() { return "int8_t"; }
template <>
const char* getclassname<char>() { return "char"; }
template <>
const char* getclassname<int16_t>() { return "int16_t"; }
template <>
const char* getclassname<int32_t>() { return "int32_t"; }
template <>
const char* getclassname<int64_t>() { return "int64_t"; }
template <>
const char* getclassname<uint8_t>() { return "uint8_t"; }
template <>
const char* getclassname<uint16_t>() { return "uint16_t"; }
template <>
const char* getclassname<uint32_t>() { return "uint32_t"; }
template <>
const char* getclassname<uint64_t>() { return "uint64_t"; }
template <>
const char* getclassname<bool>() { return "bool"; }
template <>
const char* getclassname<std::string>() { return "std::string"; }
template <>
const char* getclassname<void*>() { return "void*"; }
template <>
const char* getclassname<void>() { return "void"; }

template<> const char* getclassname<string8>() { return "string8"; }
template<> const char* getclassname<string16>() { return "string16"; }
template<> const char* getclassname<string32>() { return "string32"; }
template<> const char* getclassname<string64>() { return "string64"; }
template<> const char* getclassname<string128>() { return "string128"; }
template<> const char* getclassname<LogString>() { return "LogString"; }
template<> const char* getclassname<smartstring>() { return "smartstring"; }

const char* PrintToChars::getNewCString() const 
{
  size_t len = this->str().size();
  char* result = new char[len+1];
  std::strncpy(result, this->str().c_str(), len+1);
  return result;
}

DUECA_NS_END;
