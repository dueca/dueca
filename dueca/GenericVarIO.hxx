/* ------------------------------------------------------------------   */
/*      item            : GenericVarIO.hxx
        made by         : Rene' van Paassen
        date            : 001005
        category        : header file
        description     :
        changes         : 001005 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef GenericVarIO_hxx
#define GenericVarIO_hxx

#include <stringoptions.h>
#include <iostream>
#include <vector>
#include <inttypes.h>
#include <includeguile.h>

#if !defined(SCM_MAJOR_VERSION)
// for legacy reasons, direct access to SCM objects is still in old
// code; make it harmless for Python scripting
struct scm_unused_struct;
typedef scm_unused_struct* SCM;
#endif

#include <dueca_ns.h>
DUECA_NS_START
/** The different kinds of variables that can be transmitted */
enum ProbeType {
  Probe_double,
  Probe_int,
  Probe_uint32_t,
  Probe_uint16_t,
  Probe_float,
  Probe_bool,
  Probe_TimeSpec,
  Probe_PeriodicTimeSpec,
  Probe_PrioritySpec,
  Probe_vstring,
  Probe_string8,
  Probe_string16,
  Probe_string32,
  Probe_string64,
  Probe_string128,
  Probe_vector_int,
  Probe_vector_float,
  Probe_vector_double,
  Probe_vector_vstring,
  Probe_ScriptCreatable,
  Probe_SCM,
  Probe_GenericPacker,  ///< Not generated
  Probe_Sentinel
};

/** Primt a (nice description of) the probe type. */
std::ostream& operator << (std::ostream& os, const ProbeType& p);

// forward declarations etc
typedef std::vector<int> vector_int;
typedef std::vector<double> vector_double;
typedef std::vector<float> vector_float;
typedef std::vector<vstring> vector_vstring;
class TimeSpec;
class PeriodicTimeSpec;
class PrioritySpec;
class ScriptCreatable;

template<typename T>
struct typeflag
{ };


/** A class to peek and poke into the variables of another class.

    This class is used in the specification of an inco
    calculation. Upon inco calculation, first the controls and
    constraints are poked into the inco state and input, then the inco
    calculation takes place, and the targets are peeked back. This
    class provides a generic interface to do the peeking and poking. A
    specialization of this class implements the actual work */

class GenericVarIO
{
protected:
  /// type of argument passed/inserted/read
  ProbeType ptype;

public:
  /// constructor
  GenericVarIO();

  /// destructor
  virtual ~GenericVarIO();

  /// returns the argument type required
  inline ProbeType getType() const {return ptype;}

public:

  /** \macro
      This is an auxiliary macro that defines the poke and peek
      methods, and the identifying ProbeType function for a certain
      class "A" */
#define DECLARE_IN_GENERICVAR_IO(A) \
  virtual bool poke(void* obj, const A & v) const; \
  virtual bool peek(void* obj, A & v) const; \
  static inline ProbeType getProbeType(const typeflag<A> &i)        \
  { return Probe_ ## A ;}

  /// Use handy macro to declare stuff for int
  DECLARE_IN_GENERICVAR_IO(int);
  /// Use handy macro to declare stuff for int
  DECLARE_IN_GENERICVAR_IO(uint32_t);
  /// Use handy macro to declare stuff for int
  DECLARE_IN_GENERICVAR_IO(uint16_t);
  /// Use handy macro to declare stuff for double
  DECLARE_IN_GENERICVAR_IO(double);
  /// Use handy macro to declare stuff for float
  DECLARE_IN_GENERICVAR_IO(float);
  /// Use handy macro to declare stuff for bool
  DECLARE_IN_GENERICVAR_IO(bool);
  /// Use handy macro to declare stuff for TimeSpec
  DECLARE_IN_GENERICVAR_IO(TimeSpec);
  /// Use handy macro to declare stuff for PeriodicTimeSpec
  DECLARE_IN_GENERICVAR_IO(PeriodicTimeSpec);
  /// Use handy macro to declare stuff for PrioritySpec
  DECLARE_IN_GENERICVAR_IO(PrioritySpec);
  /// Use handy macro to declare stuff for vstring
  DECLARE_IN_GENERICVAR_IO(vstring);
  /// Use handy macro to declare stuff for SCM
  DECLARE_IN_GENERICVAR_IO(SCM);
  /// Use handy macro to declare stuff for vector<int>
  DECLARE_IN_GENERICVAR_IO(vector_int);
  /// Use handy macro to declare stuff for vector<float>
  DECLARE_IN_GENERICVAR_IO(vector_float);
  /// Use handy macro to declare stuff for vector<double>
  DECLARE_IN_GENERICVAR_IO(vector_double);
  /// Use handy macro to declare stuff for vector<vstring>
  DECLARE_IN_GENERICVAR_IO(vector_vstring);
  /// Objects creatable from scripts
  DECLARE_IN_GENERICVAR_IO(ScriptCreatable);
  /// Dstring8
  DECLARE_IN_GENERICVAR_IO(string8);
  /// Dstring
  DECLARE_IN_GENERICVAR_IO(string16);
  /// Dstring
  DECLARE_IN_GENERICVAR_IO(string32);
  /// Dstring
  DECLARE_IN_GENERICVAR_IO(string64);
  /// Dstring
  DECLARE_IN_GENERICVAR_IO(string128);
};

DUECA_NS_END
#endif
