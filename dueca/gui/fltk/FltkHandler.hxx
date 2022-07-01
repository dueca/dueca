/* ------------------------------------------------------------------   */
/*      item            : FltkHandler.hxx
        made by         : Rene van Paassen
        date            : 040715
        category        : header file
        description     :
        changes         : 040715 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef FltkHandler_hxx
#define FltkHandler_hxx

#ifdef FltkHandler_cxx
#endif

#include "GuiHandler.hxx"

#include <dueca_ns.h>
DUECA_NS_START

/** This encapsulates the top-level handling of cooperation with the
    Fltk library. */
class FltkHandler: public GuiHandler
{
public:
  /** Constructor. */
  FltkHandler(const std::string& name);

  /** Destructor. */
  ~FltkHandler();

  /** Do toolkit initialization. */
  void init(bool xlib_lock);

  /** This function you passes control to the glut, effectively
      starting GUI processing in the low-priority thread. */
  void passControl();

  /** Nicely step out of the main loop again. */
  void returnControl();
};

DUECA_NS_END
#endif
