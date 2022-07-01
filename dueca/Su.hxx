/* ------------------------------------------------------------------   */
/*      item            : Su.hxx
        made by         : Rene van Paassen
        date            : 010620
        category        : header file
        description     :
        changes         : 010620 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Su_hxx
#define Su_hxx

#ifdef Su_cxx
#endif
#include <dueca_ns.h>
DUECA_NS_START

/** Access super user or root capabilities. Note: there is no
    thread-safety built in, use in a sensible manner. */
class Su
{
  /** Original user id. */
  int orig_user;

  /** Have already tried to get super user capabilities. */
  bool have_tried;

  /** Flag to indicate that it is possible to get su capabilities. */
  bool is_capable;

  /** No sense in having more than one of these. */
  static Su* singleton;
private:

  /** Constructor called from single() */
  Su();

  /// Copy constructor, not implemented
  Su(const Su&);

  /// Assignment, not implemented
  Su& operator = (const Su&);
public:

  /** Access the singleton. */
  static Su& single();

  /// Destructor;
  ~Su();

  /** Returns true if we are capable of getting super user properties */
  bool isCapable();

  /** Get su capabilities. */
  bool acquire();

  /** Return to normal capabilities. */
  bool revert();
};

DUECA_NS_END
#endif
