/* ------------------------------------------------------------------   */
/*      item            : DuecaGLGtk4Window.hxx
        made by         : Rene van Paassen
        date            : 180607
        category        : header file
        api             : DUECA_API
        description     :
        changes         : 180607 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#pragma once
#include <GLFW/glfw3.h>
#include <dueca_ns.h>
#include <dueca/visibility.h>
#include <string>
#include <vector>
#include <map>

DUECA_NS_START;

/** Provides a DUECA shell around a window with a glfw

    After deriving from this class, you should implement
    the GL drawing routine.

    You can also implement callbacks for reshape, initialisation and
    mouse and keyboard events.

    Also note that under "modern" desktops, you would get a Wayland
    window, and these use OpenGL ES, rather than the "classic" desktop
    OpenGL you might be used to. The shaders you use need to be
    compliant with ES, see

    [this WikiPedia page](https://en.wikipedia.org/wiki/OpenGL_Shading_Language)

    As far as I can tell it is not possible to mix this in the same
    DUECA process with X11 GL windows.
*/
class DuecaGLFWWindow
{
  /** Monitor */
  GLFWmonitor *glfw_mon;

  /** Window */
  GLFWwindow *glfw_win;

  /** Window title */
  std::string title;

  /** fullscreen? */
  bool fullscreen;

  /** Depth buffer */
  bool depth_buffer;

  /** Stencil buffer? */
  bool stencil_buffer;

  /** selected cursor */
  int cursortype;

  /** did the init */
  bool need_init;

  /** Width of the GL widget */
  int width;

  /** Height of the GL widget */
  int height;

  /** X position of the GL widget */
  int x;

  /** Y position of the GL widget */
  int y;

  /** Flag to indicate passing events. */
  bool dopass;

  /** map from glfw win id to class */
  unsigned opened_windows;

public:
  /** Constructor

      @param window_title Title for the window
      @param pass_passive For compatibility, passive movement always passed.
  */
  DuecaGLFWWindow(const char *window_title = "DUECA",
                    bool pass_passive = false,
                    bool depth_buffer = true,
                    bool stencil_buffer = false);

  /// Destructor
  ~DuecaGLFWWindow();

    /** Request full screen drawing -- or not. Can be linked to Scheme
    in a parameter table. This call is ignored for  */
  bool setFullScreen(const bool &fs = true);

  /** Set the window position, at least if the window manager will
      honour this. Can be linked to Scheme in a parameter table. Use
      this call before opening the window with openWindow. */
  bool setWindow(const std::vector<int> &wpos);

  /** Set up the window initial position and size. Honouring of
      initial position depends on the window manager. Use this call
      before opening the window with openWindow. */
  void setWindow(int posx, int poxy, int width, int height);

  /** Do the opening and displaying of the window. In your module,
      preferably place this call in the "complete()" method. At this
      time, all GL stuff is in place, while the system is normally not
      yet running real-time.
  */
  void openWindow();

  /** If position given, place the window */
  void placeWindow();

  /** Select cursor type

      <ul>
      <li> 0, no cursor
      <li> 1, the same cursor as the master (root) window
      <li> 2, a crosshair cursor
      <li> 3, a right arrow pointer
      <li> 4, a left arrow pointer
      </ul>
  */
  void selectCursor(int cursor);

  /** Indicate that a redraw of the window is needed. */
  void redraw();

  /** Swap front and back buffers. */
  DUECA_DEPRECATED("swapBuffers not needed for glfw")
  void swapBuffers();

  /** Set the graphics content as current.

      Note that this is normally not needed, in the initGL, display and
      reshape callbacks, the GC will be current. You can use this in your
      destructor, when GL objects are deleted, and you need a correct GC
      for that.

      @param do_select  When false, resets the GC */
  inline void selectGraphicsContext(bool do_select = true) { makeCurrent(); }

  /** If you want to do GL work outside the draw routine (add GL lists
      etc.) the window needs to be current. First call this routine in
      that case. */
  void makeCurrent();

public:
  /** Function to implement in a derived class. Can assume that the GL
      context is current, do not need a swap call at the end! Only draw! */
  virtual void display() = 0;

  /** Function to implement in a derived class. Called with the GL
      context current. */
  virtual void initGL();

  /** callback function, override to get notified of shape changes */
  virtual void reshape(int x, int y);

  /** callback function, override to get notified of keypresses */
  virtual void keyboard(char c, int x, int y);

  /** callback function, override to get notified of special key presses */
  virtual void special(int c, int x, int y);

  /** mouse motion, with pressed key */
  virtual void motion(int x, int y);

  /** mouse motion, nothing pressed */
  virtual void passive(int x, int y);

  /** mouse button press actions */
  virtual void mouse(int button, int state, int x, int y);
};

DUECA_NS_END;
