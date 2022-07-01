/* ------------------------------------------------------------------   */
/*      item            : Registry.hh
        made by         : Rene' van Paassen
        date            : 980304
        category        : header file
        description     :
        notes           : proposed uses:
                          1> For the registry of
                          components. (Performers or
                          Controllers). There must be a unique ID in
                          the Together object.
                          2> For the registry of channels. Channel
                          ends may exist at multiple places. There
                          will be a series of ID's in the Together
                          objects, one for all the places with a
                          channel end. The first must be the creator's
                          end. In addition there is a flag indicating
                          receiverinitiated.
        changes         : 980515 Have re-thought the strategy. It
                          would be better to have a map from NameSet
                          to index, and then store the data in a
                          vector. Data will be offered on vector index
                          anyhow, and look-up based on vector, not on
                          name, is more frequent. Has to be
                          implemented!
                          010714 This time has finally come. What an
                          old idea, I forgot I had it.
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef registry_hh
#define registry_hh

#ifdef registry_cc
#endif

#include <map>
#include <iostream>
#include <StateGuard.hxx>
#include <exception>
#include <dueca_ns.h>
#include <dueca/visibility.h>

#ifdef TEST_OPTIONS
//#define DEBUG_MEM
#endif

DUECA_NS_START

/** An exception object, to throw when the registry is used for
    critical tasks while not previously locked. */
class LNK_PUBLIC RegistryNotLocked: public std::exception
{
public:
  /** Constructor */
  RegistryNotLocked() {};

  /** To print. */
  const char* what() const throw() {return "registry unlocked";}
};

template<class UD> class registry;

/** Print to stream. */
template<class UD>
ostream& operator<< (ostream& o, const registry<UD> &reg);

/** A Functor, a class that implements one function. It decides
    whether one pointer is "less" than another one, on the basis of
    the value of the object the pointer points to. */
template<class T>
class less_ptr
{
public:
  /** Constructor. */
  less_ptr() { }

  /** Destructor. */
  ~less_ptr() { }

  /** The only use of the thing, returns true when the value pointed
      to by a is less than the value pointed to by b. */
  bool operator() (const T &a, const T &b) const {return *a < *b;}
};


/** This is a templated class that implements a two-way of access
    registry. One way is determined by the objects in the registry
    (usually on the basis of the object name, and a map of pointers is
    used to provide quick access along this dimension. The second way
    is access with an index. To allow for quick, lock-free access, to
    the index access, the size of the registry must be known in
    advance. It is the client's responsibility -- or risk -- to
    synchronise accessess with the objects value and insertion of
    objects. */
template<class UD>
class registry: public StateGuard
{
private:
  /** This is a map of pointers to the indexes in the array. */
  map<const UD*, int, less_ptr<const UD*> > index;

  /** This vector is the primary vector for the objects. Objects must,
      in their constructor, initialise to a default, void, value */
  UD* volatile v1;

  /** the size of v1. */
  volatile unsigned int v1_size;

  /** A second vector is used. Whenever the vector exceeds size, the
      second vector is initialised, the data is copied, and the map is
      reconstructed. In this way, unlocked access with the index
      remains possible. */
  UD* volatile v2;

  /** Extra cache to accommodate slow readers. */
  UD* vhold;

  /** the size of v2. */
  volatile unsigned int v2_size;

  /// Flag to indicate use of vector
  volatile bool use_primary;

  /// Highest index entered
  volatile unsigned int highest_index;

  /// One copy of an uninitialised object
  UD empty_ud;

  /// A flag to check the client locking behaviour
  mutable volatile bool locked;

public:

  /// constructor
  registry(int initialsize = 20, const char* name = "anon");

  /// Destructor.
  ~registry();

private:

  /// Copy constructor is private, no copying allowed
  registry(registry &r); // don't ever copy, don't implement this

  /// Assignment constructor private as well, not implemented
  registry& operator = (const registry& o);

public:
  /** This adds a copy of the entry at a specified place in the
      database. The client should have locked the database first! */
  void insert(unsigned int place, const UD &ud);

  /** Remove a copy of this entry. */
  void remove(unsigned int i);


  /** This returns a reference to the entry matching (in name
      normally) the entry given in the argument. The registry remains
      locked while you work on the reference, and you must unlock it
      manually with the unlock function. */
  UD& find(const UD &x) const;

  /** This returns the entry at location idx. Locking is not necessary
      for this function. */
  UD& find(unsigned int idx) const;

  /** Lock the registry before using one of the find functions. */
  inline void lock() const {accessState(); locked = true;}

  /** Unlock the registry after using one of the find functions. */
  inline void unlock() const {locked = false; leaveState();}

  /** Test whether the item at location is there. The registry should
      be locked before accessing this function. */
  bool contains(const UD& x) const;

  /** Test whether the item at location is there. Locking the registry
      is not necessary. */
  bool contains(unsigned int idx) const;

  /** Access to this is unlocked and fast */
  inline const UD& operator[] (unsigned int i) const {
    if (use_primary) {
      if (i < v1_size) return v1[i];
    }
    else {
      if (i < v2_size) return v2[i];
    }
    return empty_ud;
  }

  /** query the number of objects entered. */
  int size() const {return use_primary ? v1_size : v2_size; }

  /** important for debug purposes: ability to dump the registry to a
      stream. */
  friend ostream& operator<< <> (ostream& o, const registry<UD> &reg);
};

DUECA_NS_END
#endif


//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#ifndef registry_ii
#define registry_ii

#include "Exception.hxx"

#include <dueca_ns.h>

DUECA_NS_START
using namespace std;

template<class UD> registry<UD>::
registry(int initialsize, const char* name) :
  StateGuard(name),
  v1(new UD[initialsize]),
  v1_size(initialsize),
  v2(NULL),
  vhold(NULL),
  v2_size(0),
  use_primary(true),
  highest_index(0),
  empty_ud(),
  locked(false)
{
  for (int ii = v1_size; ii--; ) v1[ii] = empty_ud;
  leaveState();
}

template<class UD> registry<UD>::
~registry()
{
  delete[] vhold;
  delete[] v1;
  delete[] v2;
}

template<class UD>
void registry<UD>::insert(unsigned int place, const UD &ud)
{
  if (!locked) throw RegistryNotLocked();

  const UD *udptr = &ud;

  // Is this entry already present?? It should not be.
  typename map<const UD*, int, less_ptr<const UD*> >::const_iterator
    ii = index.find(udptr);
  if (ii != index.end()) {
    throw(ObjectAlreadyInRegistry(GlobalId(0,1),
                                  ud.getGlobalId()));
  }

  // apparently the stuff is new
  if (use_primary) {
    if (v1_size <= place) {

      // ooops, have to resize
      delete[] vhold; vhold = v2;
      v2_size = (v1_size * 2) > (place + 1) ? v1_size * 2 : place + 1;
#ifdef DEBUG_MEM
      cerr << "Resizing registry " << getName() << " to " << v2_size << endl;
#endif
      v2 = new UD[v2_size];
      for (unsigned int ii = v2_size; ii-- > v1_size; ) v2[ii] = empty_ud;
      index.clear();
      for (int ii = v1_size; ii--; ) {
        v2[ii] = v1[ii];
        if (v2[ii] != empty_ud) index[&(v2[ii])] = ii;
      }
      use_primary = false;
    }
  }
  else {
    if (v2_size <= place) {

      // ooops, have to resize
      delete[] vhold; vhold = v1;
      v1_size = (v2_size * 2) > (place + 1) ? v2_size * 2 : place + 1;
#ifdef DEBUG_MEM
      cerr << "Resizing registry " << getName() << " to " << v1_size << endl;
#endif
      v1 = new UD[v1_size];
      for (unsigned int ii = v1_size; ii-- > v2_size; ) v1[ii] = empty_ud;
      index.clear();
      for (int ii = v2_size; ii--; ) {
        v1[ii] = v2[ii];
        if (v1[ii] != empty_ud) index[&(v1[ii])] = ii;
      }
      use_primary = true;
    }
  }

  if (use_primary) {

    // insert into v1
    v1[place] = ud;
    index[&(v1[place])] = place;
  }
  else {

    // insert into v2
    v2[place] = ud;
    index[&(v2[place])] = place;
  }

  if (place > highest_index) highest_index = place;
}

template<class UD>
void registry<UD>::remove(unsigned int i)
{
  if (!locked) throw RegistryNotLocked();
  if (use_primary) {
    typename map<const UD*, int, less_ptr<const UD*> >::iterator
      ii = index.find(&(v1[i]));
    if (ii != index.end()) index.erase(ii);
    v1[i] = empty_ud;
  }
  else {
    typename map<const UD*, int, less_ptr<const UD*> >::iterator
      ii = index.find(&(v2[i]));
    if (ii != index.end()) index.erase(ii);
    v2[i] = empty_ud;
  }
}

template<class UD>
UD& registry<UD>::find(unsigned int i) const
{
  // this function should not need locking (well.., won't hurt)

  if (use_primary) {
    if (i < v1_size) {
      return v1[i];
    }
  }
  else {
    if (i < v2_size) {
      return v2[i];
    }
  }
  return const_cast<UD&>(empty_ud);
}

template<class UD>
UD& registry<UD>::find(const UD &ud) const
{
  // locking should be done by the client!
  // just to enforce this
  if (!locked) throw RegistryNotLocked();
  const UD *udptr = &ud;

  typename map<const UD*, int, less_ptr<const UD*> >::const_iterator
    ii = index.find(udptr);
  if (ii != index.end()) {
    unsigned int i = ii->second;

    if (use_primary) {
      if (i < v1_size) {
        return *(&(v1[i]));
      }
    }
    else {
      if (i < v2_size) {
        return *(&(v2[i]));
      }
    }
  }
  return const_cast<UD&>(empty_ud);
}

template <class UD>
bool registry<UD>::contains(const UD& x) const
{
  if (!locked) throw RegistryNotLocked();
  bool result;
  result = index.find(&x) != index.end();
  return result;
}

template <class UD>
bool registry<UD>::contains(unsigned int idx) const
{
  if (use_primary) {
    if (idx < v1_size) return v1[idx] != empty_ud;
  }
  else {
    if (idx < v2_size) return v2[idx] != empty_ud;
  }
  return false;
}

template<class UD>
ostream& operator << (ostream& o, const registry<UD> &reg)
{
  o << "registry dump:" << reg.highest_index << '\n';
  for (unsigned int ii = 0; ii <= reg.highest_index ; ii++) {
    if ((reg.use_primary ? reg.v1[ii] : reg.v2[ii]) != reg.empty_ud) {
      o << reg.use_primary ? reg.v1[ii] : reg.v2[ii] << '\n';
    }
    else
      o << "gap!\n";
  }
  return o;
}

DUECA_NS_END

#endif
#endif




