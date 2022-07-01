/* ------------------------------------------------------------------   */
/*      item            : GluiHandler.hxx
        made by         : Rene van Paassen
        date            : 010322
        category        : header file
        description     :
        changes         : 010322 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GluiHandler_hxx
#define GluiHandler_hxx

#include "GuiHandler.hxx"

#include <dueca_ns.h>

DUECA_NS_START
/** This encapsulates the top-level handling of cooperation with the
    Glut + Gui interface library. */
class GluiHandler: public GuiHandler
{
  /** Flag to remember whether the gui is already started. */
  bool no_gui;

public:
  /** Constructor. */
  GluiHandler(const std::string& name);

  /** Destructor. */
  ~GluiHandler();

  /** Do toolkit initialization. */
  void init(bool xlib_lock);

  /** This function you passes control to the glut, effectively
      starting GUI processing. */
  void passControl();

  /** Nicely step out of the main loop again. */
  void returnControl();
};


DUECA_NS_END
#endif
