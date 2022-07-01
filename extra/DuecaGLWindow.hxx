/* ------------------------------------------------------------------   */
/*      item            : DuecaGLWindow.hxx
        made by         : Rene van Paassen
        date            : 010409
        category        : header file
        description     :
        changes         : 010409 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef DuecaGLWindow_hxx
#define DuecaGLWindow_hxx

#include <dueca_ns.h>


// this define will be present using dueca-config with the --gtk3 flag
#ifdef DUECA_CONFIG_GTK3

#include <gtk/gtk.h>

/** @TODO: for now, the bare GL window is used. The Gtk3 GL code is not
    compatible with a lot of our old API gl drawing */
#if GTK_CHECK_VERSION(3, 16, 0)  && 0

#include <extra/gui/gtk3/DuecaGLGtk3Window.hxx>
//DUECA_NS_START;
//typedef DuecaGLGtk3Window DuecaGLWindow;
#define DuecaGLWindow DuecaGLGtk3Window
//DUECA_NS_END;
//typedef DuecaGLGtk3Window DuecaGLWindow;
#else
#warning "Using compatibility BareDuecaGLWindow for GL under gtk+3"

// gtk3 GL interfacing simplified with BareDuecaGLWindow
#include <extra/gui/X11/BareDuecaGLWindow.hxx>
//DUECA_NS_START;
//typedef BareDuecaGLWindow DuecaGLWindow;
//DUECA_NS_END;
#define DuecaGLWindow BareDuecaGLWindow
#endif

#else

// anything older than GTK3 gets the traditional complicated DuecaGLWindow

#include "DuecaGLCanvas.hxx"
#include <stringoptions.h>
#include <vector>
#include <dueca/visibility.h>

DUECA_NS_START;

/** A DuecaGLWindow is the interface between the DUECA world and
    OpenGL drawing. Independently of the windowing toolkit that is
    used -- provided that this toolkit is GL capable -- you can use
    the DuecaGLWindow to open and manipulate a GL view. */
class DuecaGLWindow : public DuecaGLCanvas
{
  /// title for the window
  vstring window_title;

  /** Also return passive mouse motion. */
  bool mouse_passive;

  /// If true, full screen draw
  bool                                       fullscreen;
  /// X-offset of the screen
  int                                        offset_x;
  /// Y-offset of the screen
  int                                        offset_y;
  /// X-size of the screen
  int                                        size_x;
  /// Y-size of the screen
  int                                        size_y;

  /** Also the Fl window needs special access. */
  friend class FlBasedGLWindow;

  /** Notification that the window is being destroyed. */
  void windowDestroyed();

public:
  /** Constructor with the possibility to specify default screen size
      and location. This defaults to windowed mode, full screen (with
      setFullScreen() call) is still possible.
      \param window_title Title for the window (shown if not
      full-screen)
      \param pass_passive If true, also passes passive (unclicked)
      mouse motions.
  */
  DuecaGLWindow(const char* window_title,
                bool pass_passive = false);

  /// @cond DO_NOT_DOCUMENT
  DUECA_DEPRECATED("please used the new constructor signature")
  /// @endcond DO_NOT_DOCUMENT
  /** Backwards compatible constructor. Arguments, except
      mouse_passive, are ignored. */
  DuecaGLWindow(const char* window_title,
                bool dummy1,
                bool dummy2,
                bool dummy3 = true,
                bool dummy4 = true,
                bool mouse_passive = true);

  /** destructor. */
  virtual ~DuecaGLWindow();

  /** Request full screen drawing -- or not. Can be linked to Scheme
      in a parameter table. Use this call before opening the window
      with openWindow. */
  bool setFullScreen(const bool& fs = true);

  /** Set the window position, at least if the window manager will
      honour this. Can be linked to Scheme in a parameter table. Use
      this call before opening the window with openWindow. */
  bool setWindow(const std::vector<int>& wpos);

  /** Set up the window initial position and size. Honouring of
      initial position depends on the window manager. Use this call
      before opening the window with openWindow. */
  void setWindow(int posx, int poxy, int width, int height);

  /** Do the opening and displaying of the window. In your module,
      preferably place this call in the "complete()" method. At this
      time, all GL stuff is in place, while the system is normally not
      yet running real-time.
      @param priority  priority level for the manager/thread; used to find
                       the protocol matching the "sweeper"
  */
  void openWindow(int priority = -1);

  /** Close the window again. */
  void hide();

  /** Open the window, after a hide. */
  void show();

  /** Get x offset */
  int getXOffset() const {return offset_x;}
  /** Get y offset */
  int getYOffset() const {return offset_y;}
public:
  /** \{ Prototypes for interation handling.
  This class adheres to -- as much as is practical -- the calling
  conventions for Glut. */

  /** Called when a key is pressed. */
  virtual void keyboard(unsigned char key, int x, int y);

  /** Called when a function key is pressed.
  \todo Does not currently work under gtk. */
  virtual void special(int key, int x, int y);

  /** This is called whenever a mouse button event comes in */
  virtual void mouse(int button, int state, int x, int y);

  /** This is called whenever a mouse motion event comes in. */
  virtual void motion(int x, int y);

  /** This is called whenever a mouse motion event comes in, but none
  of the buttons are pressed. */
  virtual void passive(int x, int y);

  /** \} */

  /** get the flag that determines whether passive mouse motions are
  to be given. */
  inline bool passPassive() { return mouse_passive; }
};

DUECA_NS_END;
#endif
#endif
