/* ------------------------------------------------------------------   */
/*      item            : DataClassRegistrar.cxx
        made by         : Rene' van Paassen
        date            : 130118
        category        : body file
        description     :
        changes         : 130118 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DataClassRegistrar_cxx
#include "DataClassRegistrar.hxx"
#include "DataClassRegistry.hxx"
DUECA_NS_START;

DataClassRegistrar::DataClassRegistrar(const char* classname,
                                       const char* parent,
                                       const CommObjectDataTable* table,
                                       const functortable_type* functortable,
                                       const DataSetConverter* converter)
{
  DataClassRegistry::single().registerClass
    (classname, parent, table, functortable, converter);
}
DUECA_NS_END;

