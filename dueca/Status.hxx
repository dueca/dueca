/* ------------------------------------------------------------------   */
/*      item            : ModuleStatus.hxx
        made by         : Rene van Paassen
        date            : 010822
        category        : header file
        description     :
        changes         : 010822 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ModuleStatus_hxx
#define ModuleStatus_hxx

#ifdef ModuleStatus_cxx
#endif

#include <GenericStatus.hxx>
#include <dueca_ns.h>
DUECA_NS_START

/** This summarises the state of a DUECA module. */
template <class S>
class Status : public GenericStatus
{
  /** Status object. */
  S status;

  /** Combine this status with another one (must be of the same type). */
  void operatorAndEq(const GenericStatus& o);

public:
  /** Constructor. */
  Status();

  /** Constructor with a default argument. */
  Status(const S &status);

  /** Copy constructor. */
  Status(const Status& o);

  /** Destructor. */
  ~Status();

  /** Clear status, neutral or something. */
  void clear();

  /** Equality test. */
  bool operator == (const GenericStatus& o) const;

  /** Make a new object, of the same type and identical to this one. */
  GenericStatus* clone() const;

  /** Print in a nice format to some stream. */
  ostream& print(ostream& os) const;
};

DUECA_NS_END
#endif
