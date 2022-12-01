/* ------------------------------------------------------------------   */
/*      item            : DataClassRegistrar.hxx
        made by         : Rene van Paassen
        date            : 130118
        category        : header file
        description     :
        changes         : 130118 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataClassRegistrar_hxx
#define DataClassRegistrar_hxx

#include <dueca_ns.h>
#include <inttypes.h>
#include <map>
#include <string>
#include <memory>
#include <dueca_ns.h>

DUECA_NS_START;
class DataSetConverter;
struct CommObjectDataTable;
class DCOMetaFunctor;
typedef std::map<std::string,std::shared_ptr<dueca::DCOMetaFunctor> >
functortable_type;

/** Helper class to register all communicatable channel object
    types. The creation of a static object of these types registers
    the different classes with a registry. */
class DataClassRegistrar
{
public:
  /** Constructor
      \param classname   Name of to-be registered class
      \param parent      Parent, or NULL if none
      \param table       Table for extraction of data members
      \param converter   Data set converter */
  DataClassRegistrar(const char* const classname, const char* const parent,
                     const CommObjectDataTable* table,
                     const functortable_type* functortable,
                     const DataSetConverter* converter);
};

DUECA_NS_END;
#endif
