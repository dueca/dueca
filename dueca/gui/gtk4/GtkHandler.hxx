/* ------------------------------------------------------------------   */
/*      item            : GtkHandler.hxx
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

#ifndef GtkHandler_hxx
#define GtkHandler_hxx

#ifdef GtkHandler_cxx
#endif

#include "GuiHandler.hxx"
#include <gtk/gtk.h>

#include <dueca_ns.h>
DUECA_NS_START
/** This encapsulates the top-level handling of cooperation with the
    Gtk interface library */
class GtkHandler: public GuiHandler
{
  GtkApplication *app;
public:
  /** Constructor. */
  GtkHandler(const std::string& name);

  /** Destructor. */
  ~GtkHandler();

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
