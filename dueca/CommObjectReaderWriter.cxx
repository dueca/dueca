/* ------------------------------------------------------------------   */
/*      item            : CommObjectReaderWriter.cxx
        made by         : Rene' van Paassen
        date            : 141202
        category        : body file
        description     :
        changes         : 141202 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define CommObjectReaderWriter_cxx
#include "CommObjectReaderWriter.hxx"
#include <CommObjectMemberAccess.hxx>
#include <DataClassRegistry.hxx>

DUECA_NS_START;

CommObjectReaderWriter::CommObjectReaderWriter(const char* classname) :
  entry(DataClassRegistry::single().getEntry(classname))
{
  //
}

CommObjectReaderWriter::~CommObjectReaderWriter()
{

}

const char* CommObjectReaderWriter::getMemberName(unsigned i) const
{
  return DataClassRegistry::single().getMemberName(entry, i);
}

const char* CommObjectReaderWriter::getMemberClass(unsigned i) const
{
  return DataClassRegistry::single().getMemberAccessor(entry, i)
    ->getClassname();
}

const char* CommObjectReaderWriter::getMemberKeyClass(unsigned i) const
{
  return DataClassRegistry::single().getMemberAccessor(entry, i)
    ->getKeyClassname();
}

MemberArity CommObjectReaderWriter::getMemberArity(unsigned i) const
{
  return DataClassRegistry::single().getMemberAccessor(entry, i)
    ->getArity();
}

size_t CommObjectReaderWriter::getMemberSize(unsigned i) const
{
  return DataClassRegistry::single().getMemberAccessor(entry, i)
    ->getSize();
}

size_t CommObjectReaderWriter::getNumMembers() const
{
  return DataClassRegistry::single().getNumMembers(entry);
}

const char* CommObjectReaderWriter::getClassname() const
{
  return DataClassRegistry::single().getEntryClassname(entry);
}

const CommObjectMemberAccessBase& CommObjectReaderWriter::getMemberAccessor(unsigned i) const
{
  return *DataClassRegistry::single().getMemberAccessor(entry, i);
}

CommObjectReaderWriter& CommObjectReaderWriter::operator =
(const CommObjectReaderWriter& o)
{
  this->entry = o.entry;
  return *this;
}


DUECA_NS_END;
