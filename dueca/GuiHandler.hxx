/* ------------------------------------------------------------------   */
/*      item            : GuiHandler.hxx
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

#ifndef GuiHandler_hxx
#define GuiHandler_hxx

#include <dueca_ns.h>
#include <string>
#include <map>
#include <list>

DUECA_NS_START

/** c-level call to initialize stuff with gui init. */
typedef void (* initf) (const std::string &);

/** This encapsulates top-level control of the applicable GUI
    interface. Derived classes implement actual interfaces, Glut, Gtk
    or others. This class by itself implements the "default" case,
    i.e. no GUI at all. */
class GuiHandler
{
  /** Init functions for other components. */
  static std::list<initf> &hooks();

protected:
  /** Check flag, to check for hooks added after run */
  static bool hooks_done;

  /** Flag to indicate that we are currently looping within the gui */
  bool in_gui;

  /** Special case, has glut been initialised? */
  static bool glut_initialised;

  /** run additional init code. */
  void runHooks();

  std::string guiname;

public:
  /** Map with all created handlers. */
  static std::map<std::string, GuiHandler*>& all();

  /** Creator, should initialise the GUI */
  GuiHandler(const std::string& name);

  /** Destructor, should terminate the GUI. */
  virtual ~GuiHandler();

  /** Do toolkit initialization. */
  virtual void init(bool xlib_lock);

  /** This function you passes control to the glut, effectively
      starting GUI processing. */
  virtual void passControl();

  /** Nicely step out of the main loop again. */
  virtual void returnControl();

  /** Add a callback function for calling at initialization. */
  static void addInitHook(initf hook);

  /** Keep track of whether glut has to be initialised. */
  static bool haveToInitialiseGlut()
  {
    if (glut_initialised) return false;
    glut_initialised = true; return glut_initialised;
  }
};

DUECA_NS_END
#endif
