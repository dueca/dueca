/* ------------------------------------------------------------------   */
/*      item            : PythonCorrectedName.hxx
        made by         : Rene' van Paassen
        date            : 220204
        category        : body file
        description     :
        changes         : 220204 first version
        language        : C++
        copyright       : (c) 2022 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "PythonCorrectedName.hxx"

DUECA_NS_START


PythonCorrectedName::PythonCorrectedName(const char* given) : name(given)
{
  if (given == NULL) {
    throw(creationexception("Please supply a name for your scripted class"));
  }
  const std::string toreplace("!$%&*+-./:<=>?@^~");
  for (size_t pos = name.size(); pos--; ) {
    if (toreplace.find(name[pos]) != std::string::npos) {
      name.replace(pos, 1, "_");
    }
  }
}

DUECA_NS_END

