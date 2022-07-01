/* ------------------------------------------------------------------   */
/*      item            : CommObjectReader.cxx
        made by         : Rene' van Paassen
        date            : 131202
        category        : body file
        description     :
        changes         : 131202 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define CommObjectReader_cxx
#include <CommObjectReader.hxx>
#include <CommObjectElementReader.hxx>
#include <CommObjectMemberAccess.hxx>
#include <DataClassRegistry.hxx>

DUECA_NS_START;

CommObjectReader::CommObjectReader(const char* classname, const void* obj) :
  CommObjectReaderWriter(classname),
  obj(obj)
{

}

ElementReader CommObjectReader::operator [] (const char* ename)
{
  return DataClassRegistry::single().getMemberAccessor(entry, ename)
    ->getReader(obj);
}

ElementReader CommObjectReader::operator [] (unsigned i)
{
  return DataClassRegistry::single().getMemberAccessor(entry, i)
    ->getReader(obj);
}

CommObjectReader::~CommObjectReader()
{

}

DCOReader::DCOReader(const char* classname,
                     ChannelReadToken &token, const DataTimeSpec& ts) :
  CommObjectReader(classname, NULL),
  token(token),
  ts_request(ts.getValidityStart())
{ access(); }

DCOReader::DCOReader(const char* classname,
                     ChannelReadToken &token, const TimeSpec& ts) :
  CommObjectReader(classname, NULL),
  token(token),
  ts_request(ts.getValidityStart())
{ access(); }

DCOReader::DCOReader(const char* classname, ChannelReadToken &token,
                     TimeTickType ts) :
  CommObjectReader(classname, NULL),
  token(token),
  ts_request(ts)
{ access(); }

DCOReader::~DCOReader()
{
  if (obj) {
    token.releaseAccess(obj);
  }
}

#if 0
ElementReader DCOReader::operator [] (const char* ename)
{
  //access();
  return CommObjectReader::operator [] (ename);
}

ElementReader DCOReader::operator [] (unsigned i)
{
  //access();
  return CommObjectReader::operator [] (i);
}
#endif

void DCOReader::access()
{
  if (obj != NULL) return;
  obj = token.getAccess(ts_request, ts_data, data_origin,
                        DataClassRegistry::single().getMagic(entry));
  if (!obj) throw(NoDataAvailable(token.getChannelId(), token.getClientId()));
}

DUECA_NS_END;
