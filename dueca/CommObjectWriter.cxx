/* ------------------------------------------------------------------   */
/*      item            : CommObjectWriter.cxx
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

#define CommObjectWriter_cxx
#include <CommObjectWriter.hxx>
#include <CommObjectElementWriter.hxx>
#include <CommObjectMemberAccess.hxx>
#include <ChannelWriteToken.hxx>
#include <DataClassRegistry.hxx>

DUECA_NS_START;

CommObjectWriter::CommObjectWriter(const char* classname, void* obj) :
  CommObjectReaderWriter(classname),
  obj(obj)
{
  //
}

ElementWriter CommObjectWriter::operator [] (const char* ename)
{
  return DataClassRegistry::single().getMemberAccessor(entry, ename)
    ->getWriter(obj);
}

ElementWriter CommObjectWriter::operator [] (unsigned i)
{
  return DataClassRegistry::single().getMemberAccessor(entry, i)
    ->getWriter(obj);
}

CommObjectWriter::~CommObjectWriter()
{
  //
}

CommObjectWriter& CommObjectWriter::operator = (const CommObjectWriter& o)
{
  CommObjectReaderWriter::operator = (o);
  this->obj = o.obj;
  return *this;
}

DCOWriter::DCOWriter(const char* classname,
                     ChannelWriteToken &token, const DataTimeSpec& ts) :
  CommObjectWriter(classname, NULL),
  ts_write(ts),
  token(token),
  a_ok(true)
{
  obj = token.getAccess(DataClassRegistry::single().getMagic(entry));
}

DCOWriter::DCOWriter(ChannelWriteToken &token, const DataTimeSpec& ts) :
  CommObjectWriter(token.getDataClassName().c_str(), NULL),
  ts_write(ts),
  token(token)
{
  obj = token.getAccess(DataClassRegistry::single().getMagic(entry));
}

DCOWriter::DCOWriter(ChannelWriteToken &token,
                     TimeTickType ts) :
  CommObjectWriter(token.getDataClassName().c_str(), NULL),
  ts_write(ts, ts),
  token(token)
{
  obj = token.getAccess(DataClassRegistry::single().getMagic(entry));
}

DCOWriter::~DCOWriter()
{
  if (a_ok) {
    token.releaseAccess(obj, ts_write);
  }
  else {
    token.discardAccess(obj);
  }
}

DUECA_NS_END;
