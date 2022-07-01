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

#ifndef LogCategory_hxx
#define LogCategory_hxx

#include <map>
#include <iostream>
#include <AmorphStore.hxx>
#include <dueca_ns.h>
#include <CommObjectTraits.hxx>

DUECA_NS_START

/** Defines different possible categories for logging data. Note that for
    packing/unpacking this one is treated as a basic type; has no name. */
class LogCategory
{
private:
  /** Short, printable mnemonic for the category, 4chars max. */
  union {
    uint32_t i;
    char  name[5];
  } id;

  /** Singleton method to access the map */
  static std::map<uint32_t,vstring>& explain();

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
  inline const char* getName() const { return id.name; }

  /** Get id. */
  inline const uint32_t getId() const { return id.i; }

public:
  /** Is equal to? */
  inline bool operator == (const LogCategory& o) const {return id.i == o.id.i;}

  /** Is not equal to? */
  inline bool operator != (const LogCategory& o) const {return id.i != o.id.i;}

  /** Lexical comparison. */
  inline bool operator < (const LogCategory& o) const {return id.i < o.id.i;}
};

template<>
const char* getclassname<LogCategory>();

DUECA_NS_END

inline void packData(DUECA_NS::AmorphStore &s, const DUECA_NS::LogCategory& l)
{ l.packData(s); }
inline void unPackData(DUECA_NS::AmorphReStore &s, DUECA_NS::LogCategory& l)
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

#endif

