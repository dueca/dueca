/* ------------------------------------------------------------------   */
/*      item            : OpenGLHelper.hxx
        made by         : Rene van Paassen
        date            : 060531
        category        : header file
        description     : Common interface class to glut, gtk or
                          whatever-based opengl windowing code
        changes         : 060531 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef OpenGLHelper_hxx
#define OpenGLHelper_hxx

#include <string>
#include <map>
#include <dueca_ns.h>

DUECA_NS_START;

class DuecaGLWindow;

/** This class defines the basic opengl operations. Derived classes
    implement these for specific windowing interfaces. */
class GLWindowHelper
{
public:
  /** Hide the window. */
  virtual void hide() = 0;
  /** Show the window. */
  virtual void show() = 0;
  /** Make the window's gl context current. */
  virtual void current() = 0;
  /** Change the window's cursor type. */
  virtual void cursor(int i) = 0;
  /** Schedule a redraw of the window. */
  virtual void redraw() = 0;
  /** Schedule buffer swap after redraw. */
  virtual void swap() = 0;
  /** Return the width of the window. */
  virtual int width() = 0;
  /** Return the height of the window. */
  virtual int height() = 0;
  /** Return the x offset of the window. */
  virtual int xoffset() = 0;
  /** Return the y offset of the window. */
  virtual int yoffset() = 0;
  /** Open the window. */
  virtual void open(const std::string &title, DuecaGLWindow* master,
                    int offset_x, int offset_y, int width, int height,
                    bool fullscreen = false);
  /** Constructor. */
  GLWindowHelper();
  /** Delete the window. */
  virtual ~GLWindowHelper();
};

/** This class provides an interface to duecaGLwindow for creation of
    windows under different windowing (toolkit) regimes. */
class OpenGLHelper
{
  /** If -1, this helper lives in the default (0) thread, and is handled
      by the current graphics code. Otherwise, it is a helper for a
      "sweeper". */
  int priority;
public:
  /** Map with all created helpers. */
  static std::map<std::string, OpenGLHelper*>& all();

  /** Constructor. */
  OpenGLHelper(const std::string &name);

  /** Create window. */
  virtual GLWindowHelper* newWindow() = 0;

  /** Is there a "sweeper", interacting with the GL toolkit, installed?
      returns true if the sweeper matches the caller's priority level */
  inline bool haveSweeper(int priority = -1)
  {
    return (this->priority >= 0 &&
            (priority == -1 || priority == this->priority));
  }

  /** Declare that this handler is claimed by a sweeper.
  \param priority  Thread priority of the sweeper
  \returns         True if the sweeper can be installed. */
  bool setSweeper(int priority);

  /** Destructor. */
  virtual ~OpenGLHelper();
};

DUECA_NS_END;

#endif
