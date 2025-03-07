/* ------------------------------------------------------------------   */
/*      item            : CommObjectReaderWriter.hxx
        made by         : Rene van Paassen
        date            : 141202
        category        : header file
        description     :
        changes         : 141202 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectReaderWriter_hxx
#define CommObjectReaderWriter_hxx

#include <dueca_ns.h>
#include <DataClassRegistryPredef.hxx>
#include <CommObjectExceptions.hxx>

DUECA_NS_START;

/** Base class with common elements for both anonymous/introspective
    read and write access to channel data.
*/
class CommObjectReaderWriter
{
protected:
  /** entry giving the class information */
  DataClassRegistry_entry_type entry;

public:
 /** Constructor, for testing purposes, and for recursively accessing
      complex objects.

      @param classname Type of data; must match, or the result is
                       nonsense!
  */
  CommObjectReaderWriter(const char* classname);

  /** Destructor */
  ~CommObjectReaderWriter();

  /** Return an element name based on index.
      @throws DataClassMemberNotFound
  */
  const char* getMemberName(unsigned i) const;

  /** Return an element's class based on index.
      @throws DataClassMemberNotFound
  */
  const char* getMemberClass(unsigned i) const;

  /** Return an element's key class based on index.
      @throws DataClassMemberNotFound
  */
  const char* getMemberKeyClass(unsigned i) const;

  /** Return the member arity */
  MemberArity getMemberArity(unsigned i) const;

  /** Get member fixed length, if available */
  size_t getMemberSize(unsigned i) const;

  /** Return the classname of the currently written or read type;
      typically for debugging messages */
  const char* getClassname() const;

  /** Determine the number of members in the object */
  size_t getNumMembers() const;

  /** Directly reach the MemberAccess object */
  const CommObjectMemberAccessBase &getMemberAccessor(unsigned i) const;

  /** assignment, needed for temporary copy MSGPACKtoDCO */
  CommObjectReaderWriter& operator = (const CommObjectReaderWriter& o);
};

DUECA_NS_END;

#endif
