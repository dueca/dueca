/* ------------------------------------------------------------------   */
/*      item            : HDF5Templates.cxx
        made by         : Rene' van Paassen
        date            : 170325
        category        : body file
        description     :
        changes         : 170325 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2 - Rene van Paassen
*/

#define HDF5Templates_cxx
#include "HDF5Templates.hxx"

DUECA_NS_START;

template<> const H5::DataType* get_hdf5_type(const double& t)
{ return get_hdf5_type<double>(); }
template<> const H5::DataType* get_hdf5_type(const float& t)
{ return get_hdf5_type<float>(); }
template<> const H5::DataType* get_hdf5_type(const int32_t& t)
{ return get_hdf5_type<int32_t>(); }
template<> const H5::DataType* get_hdf5_type(const uint32_t& t)
{ return get_hdf5_type<uint32_t>(); }
template<> const H5::DataType* get_hdf5_type(const int64_t& t)
{ return get_hdf5_type<int64_t>(); }
template<> const H5::DataType* get_hdf5_type(const uint64_t& t)
{ return get_hdf5_type<uint64_t>(); }
template<> const H5::DataType* get_hdf5_type(const int16_t& t)
{ return get_hdf5_type<int16_t>(); }
template<> const H5::DataType* get_hdf5_type(const uint16_t& t)
{ return get_hdf5_type<uint16_t>(); }
template<> const H5::DataType* get_hdf5_type(const unsigned char& t)
{ return get_hdf5_type<unsigned char>(); }
template<> const H5::DataType* get_hdf5_type(const char& t)
{ return get_hdf5_type<char>(); }
template<> const H5::DataType* get_hdf5_type(const bool& t)
{ return get_hdf5_type<bool>(); }
#if 1
template<> const H5::DataType* get_hdf5_type(const std::string& t)
{ return get_hdf5_type<std::string>(); }
template<> const H5::DataType* get_hdf5_type(std::string& t)
{ return get_hdf5_type<std::string>(); }
template<> const H5::DataType* get_hdf5_type(const smartstring& t)
{ return get_hdf5_type<smartstring>(); }
template<> const H5::DataType* get_hdf5_type(smartstring& t)
{ return get_hdf5_type<smartstring>(); }
#endif

template<> const H5::DataType* get_hdf5_type(double& t)
{ return get_hdf5_type<double>(); }
template<> const H5::DataType* get_hdf5_type(float& t)
{ return get_hdf5_type<float>(); }
template<> const H5::DataType* get_hdf5_type(int32_t& t)
{ return get_hdf5_type<int32_t>(); }
template<> const H5::DataType* get_hdf5_type(uint32_t& t)
{ return get_hdf5_type<uint32_t>(); }
template<> const H5::DataType* get_hdf5_type(int64_t& t)
{ return get_hdf5_type<int64_t>(); }
template<> const H5::DataType* get_hdf5_type(uint64_t& t)
{ return get_hdf5_type<uint64_t>(); }
template<> const H5::DataType* get_hdf5_type(int16_t& t)
{ return get_hdf5_type<int16_t>(); }
template<> const H5::DataType* get_hdf5_type(uint16_t& t)
{ return get_hdf5_type<uint16_t>(); }
template<> const H5::DataType* get_hdf5_type(unsigned char& t)
{ return get_hdf5_type<unsigned char>(); }
template<> const H5::DataType* get_hdf5_type(char& t)
{ return get_hdf5_type<char>(); }
template<> const H5::DataType* get_hdf5_type(bool& t)
{ return get_hdf5_type<bool>(); }

//template<unsigned N> const H5::DataType* get_hdf5_type(const Dstring<N>& t)
//{ return get_hdf5_type<Dstring<N> >(); }


template<> const H5::DataType* get_hdf5_type<double>()
{
  static const H5::PredType data_type = H5::PredType::NATIVE_DOUBLE;
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<float>()
{
  static const H5::PredType data_type = H5::PredType::NATIVE_FLOAT;
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<int32_t>()
{
  static const H5::PredType data_type = H5::PredType::NATIVE_INT32;
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<uint32_t>()
{
  static const H5::PredType data_type = H5::PredType::NATIVE_UINT32;
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<int64_t>()
{
  static const H5::PredType data_type = H5::PredType::NATIVE_INT64;
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<uint64_t>()
{
  static const H5::PredType data_type = H5::PredType::NATIVE_UINT64;
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<int16_t>()
{
  static const H5::PredType data_type = H5::PredType::NATIVE_INT16;
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<uint16_t>()
{
  static const H5::PredType data_type = H5::PredType::NATIVE_UINT16;
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<unsigned char>()
{
  static const H5::StrType data_type(H5::PredType::NATIVE_UCHAR);
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<char>()
{
  static const H5::StrType data_type(H5::PredType::NATIVE_CHAR);
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<bool>()
{
  static const H5::PredType data_type = H5::PredType::NATIVE_HBOOL;
  return &data_type;
}

#if 1
template<> const H5::DataType* get_hdf5_type<std::string>()
{
  static const H5::StrType data_type(H5::PredType::NATIVE_CHAR, H5T_VARIABLE);
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<dueca::smartstring>()
{
  static const H5::StrType data_type(H5::PredType::NATIVE_CHAR, H5T_VARIABLE);
  return &data_type;
}
#endif

template<> const H5::DataType* get_hdf5_type<dueca::Dstring<8> >()
{
  static const H5::StrType data_type(H5::PredType::C_S1, 8);
  return &data_type;
}
template<> const H5::DataType* get_hdf5_type<dueca::Dstring<16> >()
{
  static const H5::StrType data_type(H5::PredType::C_S1, 16);
  return &data_type;
}

template<> const H5::DataType* get_hdf5_type<dueca::Dstring<32> >()
{
  static const H5::StrType data_type(H5::PredType::C_S1, 32);
  return &data_type;
}

template<> const H5::DataType* get_hdf5_type<dueca::Dstring<40> >()
{
  static const H5::StrType data_type(H5::PredType::C_S1, 40);
  return &data_type;
}

template<> const H5::DataType* get_hdf5_type<dueca::Dstring<64> >()
{
  static const H5::StrType data_type(H5::PredType::C_S1, 64);
  return &data_type;
}

template<> const H5::DataType* get_hdf5_type<dueca::Dstring<128> >()
{
  static const H5::StrType data_type(H5::PredType::C_S1, 128);
  return &data_type;
}

template<> const H5::DataType* get_hdf5_type<dueca::Dstring<256> >()
{
  static const H5::StrType data_type(H5::PredType::C_S1, 256);
  return &data_type;
}

template<> const H5::DataType* get_hdf5_type<dueca::Dstring<236> >()
{
  static const H5::StrType data_type(H5::PredType::C_S1, 236);
  return &data_type;
}


const H5::DataType* get_hdf5_type(const std::string& d)
{
  return NULL;
}

const H5::DataType* get_hdf5_elt_type(const std::string& d)
{
  static H5::StrType data_type(H5T_C_S1, H5T_VARIABLE);
  return &data_type;
}

const H5::DataType* get_hdf5_elt_type(std::string& d)
{
  static H5::StrType data_type(H5T_C_S1, H5T_VARIABLE);
  return &data_type;
}

const H5::DataType* get_hdf5_elt_type(const dueca::smartstring& d)
{
  static H5::StrType data_type(H5T_C_S1, H5T_VARIABLE);
  return &data_type;
}

const H5::DataType* get_hdf5_elt_type(dueca::smartstring& d)
{
  static H5::StrType data_type(H5T_C_S1, H5T_VARIABLE);
  return &data_type;
}


DUECA_NS_END;
