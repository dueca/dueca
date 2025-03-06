/* ------------------------------------------------------------------   */
/*      item            : LogCategory.hxx
        made by         : Rene van Paassen
        date            : 061117
        category        : header file
        description     :
        changes         : 061117 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#pragma once

#include <map>
#include <iostream>
#include <AmorphStore.hxx>
#include <dueca_ns.h>
#include <CommObjectTraits.hxx>
#include <Dstring.hxx>

DUECA_NS_START

/** Defines different possible categories for logging data. Note that for
    packing/unpacking this one is treated as a basic type; has no name. */
union LogCategory
{
  /** typedef for internal reference */
  typedef LogCategory __ThisDCOType__;

public:

  //typedef union {
  //  uint32_t i;
  //  Dstring<4> name;
  //} CatUnion;

  /** Short, 4-letter printable mnemonic for the category, 4chars. */
  Dstring<5> name;
  uint32_t i;

  /** Singleton method to access the map */
  static std::map<uint32_t,vstring>& explain();

  /** a "magic" number, hashed out of the class definition,
      that will be used to check consistency of the sent objects
      across the dueca nodes. */
  static const uint32_t magic_check_number;

public:
  /** Constructor, with full data.
      \param name    Short, max 4 character, mnemonics for the category
      \param explain Larger story. */
  LogCategory(const char* name, const char* explain);

  /** Create from storage. */
  LogCategory(AmorphReStore& s);

  /** Un-initialised. */
  LogCategory();

  /** Destructor. */
  ~LogCategory();

  /** Pack into net-compatible format. */
  void packData(AmorphStore &s) const;

  /** Unpack from net-compatible format. */
  void unPackData(AmorphReStore &s);

  /** Print. */
  void print(std::ostream& os) const;

  /** Read. */
  void read(const std::string& s);

  /** Get explanation. Not thread-safe! */
  const vstring& getExplain() const;

  /** Get short name. */
  inline const char* getName() const { return name.c_str(); }

  /** Get id. */
  inline const uint32_t getId() const { return i; }

public:
  /** Is equal to? */
  inline bool operator == (const LogCategory& o) const {return i == o.i;}

  /** Is not equal to? */
  inline bool operator != (const LogCategory& o) const {return i != o.i;}

  /** Lexical comparison. */
  inline bool operator < (const LogCategory& o) const {return i < o.i;}
};

template<>
const char* getclassname<LogCategory>();

DUECA_NS_END

inline void packData(DUECA_NS::AmorphStore &s, const DUECA_NS::LogCategory& l)
{ l.packData(s); }
inline void unPackData(DUECA_NS::AmorphReStore &s, DUECA_NS::LogCategory& l)
{ l.unPackData(s); }
inline void packDataDiff(DUECA_NS::AmorphStore &s, const DUECA_NS::LogCategory& l, const DUECA_NS::LogCategory& ref)
{ l.packData(s); }
inline void unPackDataDiff(DUECA_NS::AmorphReStore &s, DUECA_NS::LogCategory& l)
{ l.unPackData(s); }

PRINT_NS_START
inline std::ostream& operator << (std::ostream& os,
                                  const DUECA_NS::LogCategory& o)
{
  o.print(os); return os;
}
inline std::istream& operator >> (std::istream& is, DUECA_NS::LogCategory& o)
{
  std::string tmp; is >> tmp; o.read(tmp);
  return is;
}
PRINT_NS_END

// specialization of classname
template<>
const char* dueca::getclassname<dueca::LogCategory>();
template <>
inline const char* dueca::getclassname<const dueca::LogCategory>()
{ return dueca::getclassname<LogCategory>(); }

#if !defined(__DCO_STANDALONE)
namespace dueca {
/** Template specialization, defines a trait that is needed if
    LogCategory is ever used inside other dco objects. */
template <>
struct dco_nested<LogCategory> : public dco_isnested { };
};
#endif

