/* ------------------------------------------------------------------   */
/*      item            : GtkFltkGLHandler.hxx
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

#ifndef GtkFltkGLHandler_hxx
#define GtkFltkGLHandler_hxx

#ifdef GtkFltkGLHandler_cxx
#endif

#include "GuiHandler.hxx"

#include <dueca_ns.h>
DUECA_NS_START

/** This encapsulates the top-level handling of cooperation with the
    Gtk interface library, with an additional initialization of Fltk
    for the gl windows. */
class GtkFltkGLHandler: public GuiHandler
{
public:
  /** Constructor. */
  GtkFltkGLHandler(const std::string& name);

  /** Destructor. */
  ~GtkFltkGLHandler();

  /** Do toolkit initialization. */
  void init();

  /** This function you passes control to the glut, effectively
      starting GUI processing in the low-priority thread. */
  void passControl();

  /** Nicely step out of the main loop again. */
  void returnControl();
};

DUECA_NS_END
#endif
