/* ------------------------------------------------------------------   */
/*      item            : GenericVarIO.cxx
        made by         : Rene' van Paassen
        date            : 001005
        category        : body file
        description     :
        changes         : 001005 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define GenericVarIO_cc
#include "GenericVarIO.hxx"
DUECA_NS_START

using namespace std;

ostream& operator << (ostream& os, const ProbeType& p)
{
  static const char* description[int(Probe_Sentinel)] = {
    "double",
    "integer",
    "uint32_t",
    "uint16_t",
    "float",
    "boolean",
    "TimeSpec",
    "PeriodicTimeSpec",
    "PrioritySpec",
    "string",
    "string8",
    "string16",
    "string32",
    "string64",
    "string128",
    "array of integers",
    "array of floats",
    "array of doubles",
    "array of strings",
    "object derived from ScriptCreatable",
    "any scheme object",
    "GenericPacker"
  };
  if (p < Probe_Sentinel) {
    return os << description[int(p)];
  }
  return os << "unknown type";
}


GenericVarIO::GenericVarIO()
{
  //
}

GenericVarIO::~GenericVarIO()
{
  //
}

// to flag things not implemented
#define MAKE_IN_GENERICVAR_IO(A) \
bool GenericVarIO::poke(void* obj, const A & v) const \
{ return false; } \
bool GenericVarIO::peek(void* obj, A & v) const \
{ return false; }

MAKE_IN_GENERICVAR_IO(int);
MAKE_IN_GENERICVAR_IO(uint32_t);
MAKE_IN_GENERICVAR_IO(uint16_t);
MAKE_IN_GENERICVAR_IO(double);
MAKE_IN_GENERICVAR_IO(float);
MAKE_IN_GENERICVAR_IO(bool);
MAKE_IN_GENERICVAR_IO(TimeSpec);
MAKE_IN_GENERICVAR_IO(PeriodicTimeSpec);
MAKE_IN_GENERICVAR_IO(PrioritySpec);
MAKE_IN_GENERICVAR_IO(vstring);
MAKE_IN_GENERICVAR_IO(SCM);
MAKE_IN_GENERICVAR_IO(vector_int);
MAKE_IN_GENERICVAR_IO(vector_float);
MAKE_IN_GENERICVAR_IO(vector_double);
MAKE_IN_GENERICVAR_IO(vector_vstring);
MAKE_IN_GENERICVAR_IO(ScriptCreatable);
MAKE_IN_GENERICVAR_IO(string8);
MAKE_IN_GENERICVAR_IO(string16);
MAKE_IN_GENERICVAR_IO(string32);
MAKE_IN_GENERICVAR_IO(string64);
MAKE_IN_GENERICVAR_IO(string128);

DUECA_NS_END
