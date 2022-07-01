/* ------------------------------------------------------------------   */
/*      item            : PackerSet.hh
        made by         : Rene' van Paassen
        date            : 990708
        category        : header file
        description     :
        changes         : 990708 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef PackerSet_hh
#define PackerSet_hh

#ifdef PackerSet_cc
#endif

#include "TransportClass.hxx"
#include <dueca_ns.h>
#include "GenericPacker.hxx"
#include "ScriptCreatable.hxx"
#include <boost/intrusive_ptr.hpp>

DUECA_NS_START
class GenericPacker;
struct ParameterTable;

/** Combination of three packers, for the three defined transport
    priorities. */
class PackerSet:
  public ScriptCreatable
{
  /** The three different packers. \{ */
  boost::intrusive_ptr<GenericPacker> admin, regular, high_prio;  /// \}

  /** Set low-prio packer call */
  bool setAdmin(ScriptCreatable &p, bool in);
  /** Set regular-prio packer call */
  bool setRegular(ScriptCreatable &p, bool in);
  /** Set high-prio packer call */
  bool setHighPrio(ScriptCreatable &p, bool in);

public:
  /** Constructor. */
  PackerSet();

  /** Constructor with references to objects. */
  PackerSet(GenericPacker& admin,
            GenericPacker& regular,
            GenericPacker& highprio);

  /** Complete function.*/
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Destructor. */
  virtual ~PackerSet();

  /** Get the parameter table. */
  static const ParameterTable* getParameterTable();

public:
  SCM_FEATURES_DEF;

public:
  /** Return the packer associated with the given transport class. */
  GenericPacker* getPacker(TransportClass tclass) const;

  /** Print to stream, debugging. */
  friend ostream& operator << (ostream& os, const PackerSet& a);
};

DUECA_NS_END
#endif

