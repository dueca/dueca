/* ------------------------------------------------------------------   */
/*      item            : TypeCreator.cxx
        made by         : Rene' van Paassen
        date            : 990723
        category        : body file
        description     :
        changes         : 990723 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DO_INSTANTIATE
#define TypeCreator_cxx
#endif

#include "TypeCreator.hxx"
#include "DuecaEnv.hxx"

#if !defined(DO_INSTANTIATE)

DUECA_NS_START

ScriptTypeCreator::ScriptTypeCreator(const std::string& type_name,
                                     const ParameterTable* table,
				     const char* vhash) :
  GenericTypeCreator(type_name, vhash),
  ArgListProcessor(table, type_name)
{
  if (DuecaEnv::scriptInstructions(type_name)) {
    ArgListProcessor::printModuleCreationCall(std::cout, callName());
  }

}

ScriptTypeCreator::~ScriptTypeCreator()
{
  //
}

bool ScriptTypeCreator::injectValues(ArgElement::arglist_t& vals, void* object)
{
  return ArgListProcessor::injectValues(vals, object);
}

DUECA_NS_END

#endif
