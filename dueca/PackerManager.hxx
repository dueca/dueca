/* ------------------------------------------------------------------   */
/*      item            : PackerManager.hh
        made by         : Rene' van Paassen
        date            : 990624
        category        : header file
        description     :
        changes         : 990624 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef PackerManager_hh
#define PackerManager_hh

#ifdef PackerManager_cc
#endif

#include "PackerSet.hxx"
#include "TransportClass.hxx"
#include <vector>
#include <dueca_ns.h>
#include "ScriptCreatable.hxx"
#include <dueca/visibility.h>
DUECA_NS_START
struct ParameterTable;

/** This class keeps a mapping with pointers to packers, three for
    each possible destination. */
class PackerManager:
  public ScriptCreatable
{
  /** Defines a mapping of packers for the transport to other nodes. */
  typedef vector<PackerSet*> PackerMapping;

  /** The mapping. */
  PackerMapping packer_set;

  /** Pointer to the singleton for this class. */
  static PackerManager* singleton;

public:
  /** Constructor. */
  PackerManager();

  /** Destructor. */
  virtual ~PackerManager();

  /** Parameter table. */
  static const ParameterTable* getParameterTable();

#ifdef SCRIPT_SCHEME
  /** Add a whole vector of Packer sets (obsolete!) */
  bool setVector(const SCM& v);
#endif

  /** Add a packer set (one by one sets adding). */
  bool addSet(ScriptCreatable& s, bool in);

  /** Check correct creation. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Returns the single instance of this object. */
  inline static PackerManager* single() {
    if (singleton == NULL) {
      cerr << "PackerManager says: Check your dueca.cnf" << endl;
      std::exit(1); // configuration error
    }
    return singleton;
  }

public:
  SCM_FEATURES_DEF;

public:
  /** Get a pointer to the appropriate transport, for a class and a
      destination. */
  static GenericPacker* LNK_WEAK findMatchingTransport(int destination,
                                                       Channel::TransportClass tclass);

  /** Print a description to stream. */
  friend ostream& operator << (ostream& os, const
                               PackerManager& a);

  /** Stop all packers. Called at destruction time. */
  void LNK_WEAK stopPackers();
};

DUECA_NS_END
#endif
