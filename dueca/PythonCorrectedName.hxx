/* ------------------------------------------------------------------   */
/*      item            : PythonCorrectedName.hxx
        made by         : Rene' van Paassen
        date            : 220204
        category        : body file
	api             : DUECA_API
        description     :
        changes         : 220204 first version
        language        : C++
        copyright       : (c) 2022 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef PythonCorrectedName_hxx
#define PythonCorrectedName_hxx

#include <dueca/dueca_ns.h>
#include <string>
#include <exception>

DUECA_NS_START

/** Alert to a condition where the classname has not been given */
class creationexception: public std::exception
{
  /** Message */
  const char* reason;
public:
  /** Constructor

      @param reason  message
  */
  creationexception(const char* reason): reason(reason) {}

  /** Print reason */
  const char* what() const throw() { return reason; }
};


/** Convert a given class name to avoid special characters. */
class PythonCorrectedName
{
  /** Name within Python */
  std::string name;
public:
  /** Constructor

      @param given  Raw name for the Python object
  */
  PythonCorrectedName(const char* given);

  /** Return the converted/checked name */
  inline const char* c_str() const { return name.c_str(); }
};

/** Return the name of a class or function, made Python-compatible. */
template<class T>
const char* core_creator_name(const char*);


/** Returns a checked/converted python name, if not overridden.

    Override this function for any classes that you may use as a 
    base class for an object class exposed to Python
*/
template<class T>
const char* core_creator_name(const char* given)
{
  static PythonCorrectedName name(given);
  return name.c_str();
}

DUECA_NS_END

#endif
