/* ------------------------------------------------------------------   */
/*      item            : ArgListProcessor.hxx
        made by         : Rene van Paassen
        date            : 030508
        category        : header file
        description     :
        changes         : 030508 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ArgListProcessor_hxx
#define ArgListProcessor_hxx

#include "dueca_ns.h"
#include "scriptinterface.h"
#include "GenericVarIO.hxx"
#include <dueca/ArgElement.hxx>
#include <iostream>
#include <boost/any.hpp>
#include <list>
#include <string>
#include <boost/scoped_ptr.hpp>

DUECA_NS_START

struct ParameterTable;
class ScriptCreatable;
class SchemeObject;

/** Custom data */
struct ArgListProcessor_Private;

/** A class that handles the processing of lists of arguments given in
    a call in the current scripting language (currently guile). */
class ArgListProcessor
{
public:
  /** The processor may or may not allow/advise using straight lists
      of parameters instead of name-value pairs. */
  enum Strategy {
    AllowListAndPair,   /**< Silently allow both lists and name-value
                             pairs. */
    NameValuePair,      /**< Only allow name-value pairs. */
    DeprecateList,      /**< Allow both lists and name-value pairs, but
                               print a warning that the straight list
                             usage is deprecated. */
    ListWithTailAndPair /**< The last element of the list may be
                             entered as often as desired. */
  };

  /** Custom struct */
  boost::scoped_ptr<ArgListProcessor_Private>  my;

private:
  /** a table with parameter names and pointers (VarProbe) to class
      members. */
  const ParameterTable* table;

  /** This flag indicates the strategy to be followed with respect to
      allowing straight lists. */
  Strategy strategy;

  /** Name of the class or module */
  std::string name;

protected:

  /** Constructor.
      \param table Table with parameter values.
      \param strat Flag to indicate whether a single straight list
                   of parameters is also acceptable. */
  ArgListProcessor(const ParameterTable* table,
                   const std::string& name,
                   Strategy strat = NameValuePair);

  /** Destructor. */
  virtual ~ArgListProcessor();

public:

  /** Return the number of entries in the parameter table. */
  int numberParameters() const;

  /** Process a list of specifications, and convert them into an internal
      list */
  template<typename T, typename R>
  bool processList(const T& specifications,
                   ArgElement::arglist_t& processed,
                   R& reference) const;

  /** Process a list of specifications, and convert these into an
      internal list.

      @tparam D    Type of variable, dependent on script language, for
                   keyword arguments
      @tparam T    Type of container, dependent on script language, for
                   remaining straight list arguments
      @param specs Keyword arguments
      @param args  Remaining arguments
      @param processed Resulting list, with converted & processed arguments
*/
  template<typename D, typename T>
  bool processList(const D& specifications,
                   const T& args,
                   ArgElement::arglist_t& processed) const;

  /** Inject a list of processed values

      @param processed Argument values, craeted with processlist
      @param object    Void pointer to the created object
      @returns         True if all arguments accepted
   */
  bool injectValues(ArgElement::arglist_t& processed, void* object) const;

  /** Print, if this were a module, the instructions to create this module */
  void printModuleCreationCall(std::ostream& os, const char* name);

  /** Print, if this were a core object, the instructions to create object */
  void printCoreCreationCall(std::ostream& os,
                             const std::string& name,
                             const std::string& args);

  /** Print a list of the arguments, together with descriptions. */
  void printArgumentList(std::ostream& os) const;

  /** Return the type of argument for entry idx in the parameter
      table.
      \param  idx  Entry in the table.
      \returns     A pre-defined type description. */
  ProbeType findType(int idx);

  inline const std::string& getName() const {return name;}

  /** Process an int value, using the call at location idx in the
      parameter table. */
  bool processValue(void *m, int idx, int value) const;

  /** Process a double value, using the call at location idx in the
      parameter table. */
  bool processValue(void *m, int idx, double value) const;

  /** Process a bool value, using the call at location idx in the
      parameter table. */
  bool processValue(void *m, int idx, bool value) const;

  /** Process a string value, using the call at location idx in the
      parameter table. */
  bool processValue(void *m, int idx, const vstring& value) const;

  /** Process a dueca fixed-length string */
  template<unsigned mxsize>
  bool processValue(void *m, int idx, const Dstring<mxsize>& value) const;

  /** Process a vector of strings, using the call at location idx in the
      parameter table. */
  bool processValue(void *m, int idx, const std::vector<vstring>& value) const;

  template<unsigned mxsize>
  bool processValue(void *m, int idx, const std::vector<Dstring<mxsize> >& value) const;

  /** Process a vector of integers, using the call at location idx in
      the parameter table. */
  bool processValue(void *m, int idx, const std::vector<int>& value) const;

  /** Process a vector of doubles, using the call at location idx in
      the parameter table. */
  bool processValue(void *m, int idx, const std::vector<double>& value) const;

  /** Process a priority specification, using the call at location idx
      in the parameter table. */
  bool processValue(void *m, int idx, const PrioritySpec& value) const;

  /** Process a time specification, using the call at location idx in
      the parameter table. */
  bool processValue(void *m, int idx, const TimeSpec& value) const;

  /** Process a periodic time specification, using the call at
      location idx in the parameter table. */
  bool processValue(void *m, int idx, const PeriodicTimeSpec& value) const;

  /** Search for a literal symbol in the parameter table.
      \param   symbol  Name for the literal.
      \returns A non-zero index into the parameter table, or -1 if the
      entry is not available. */
  int findWithSymbol(const char* symbol) const;

private:

#ifdef SCRIPT_SCHEME
  /** Process the list as a straight 1-to-1 list of arguments. */
  bool processStraight(const SCM& specifications,
                       ArgElement::arglist_t& processed,
                       SchemeObject& ref) const;

  /** Process the list as a set of literal - value pairs. */
  bool processPairs(const SCM& specifications,
                    ArgElement::arglist_t& processed,
                    SchemeObject& ref) const;

  /** Process a generic Scheme-supplied thing. */
  bool processValue(void *m, int idx, const SCM& value) const;
#endif

  /** Process something. */
  template <typename T, typename R>
  void processSomeValue(ArgElement::arglist_t& processed, unsigned idx,
                        T& specs, R& refs) const;

  /** or with a const argument. */
  template <typename T, typename R>
  void processSomeValue(ArgElement::arglist_t& processed, unsigned idx,
                        const T& specs, R& refs) const;

  /** Process a pointer to a "creatable" class,
      using the call at location idx in the parameter table. */
  bool processValue(void *m, int idx, const ScriptCreatable& value) const;
};


DUECA_NS_END

PRINT_NS_START
/** Print to stream, that is, the call and the argument list. */
ostream& operator << (ostream& os,
                      const DUECA_NS::ArgListProcessor& p);
PRINT_NS_END

#endif
