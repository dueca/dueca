/* ------------------------------------------------------------------   */
/*      item            : DuecaGLCanvas.hxx
        made by         : Joost Ellerbroek
        date            : 100625
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 100625 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#ifndef DuecaGLCanvas_hxx
#define DuecaGLCanvas_hxx

#include <dueca_ns.h>

DUECA_NS_START;

class GLWindowHelper;

/** A DuecaGLCanvas is the interface between the DUECA world and
    OpenGL drawing. Independently of the windowing toolkit that is
    used -- provided that this toolkit is GL capable -- you can use
    the DuecaGLCanvas to open and manipulate a GL view.

    DuecaGLCanvas is not used for gtk3; to get this functionality,
    either use a full-gl DuecaGLWindow (which is implemented through
    X11 directly), or insert a GtkGLArea widget in your gtk code, and
    use the gtk3 version of DuecaGLWidget to get event callbacks and
    implement the GL drawing routine.

    Note that the toolkit usage depends on the inclusion of the proper
    code with DCOMPONENTS in your project's main Makefile, and the
    toolkit is selected with the make-environment call, in dueca.cnf.
*/
class DuecaGLCanvas
{
protected:
  /** Defines a class for implementation-dependent data. The data
      needed depends on the GUI toolset(s) supported by this build of
      DUECA. */
  GLWindowHelper *helper;

  /// Remembers whether the display was already marked for redraw
  mutable bool                               marked_for_redraw;

  /** Flag to remember whether GL initialisation has taken place */
  bool gl_initialised;

  /** redrawDone is called by the callback thingies in Gtk, Glut
      etc. Only these need access, therefore a friend is declared. */
  friend void redraw_done(DuecaGLCanvas* gw);

  /** checkAndMarkInitDone is called by the callback thingies in Gtk, Glut
      etc. Only these need access, therefore a friend is declared. */
  friend bool check_and_mark_init_done(DuecaGLCanvas* gw);

  /** Notification that the window is being destroyed. */
  void widgetDestroyed();

  /** Indicate that the redraw has taken place. */
  inline void redrawDone() { marked_for_redraw = false; }

  /** return true if GL initialisation still has to be done, mark that it
      has been done. */
  inline bool checkAndMarkInitDone()
  { if (gl_initialised) return false;
    gl_initialised = true; return true; }

public:
  /// Constructor.
  DuecaGLCanvas();

  /** destructor. */
  virtual ~DuecaGLCanvas();

  /** Select a cursor type. The following types are available:

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
  void swapBuffers();

  /** If you want to do GL work outside the draw routine (add GL lists
      etc.) the window needs to be current. First call this routine in
      that case. */
  void makeCurrent();

public:
  /** \{ Prototypes for interaction handling.
      This class adheres to -- as much as is practical -- the calling
      conventions for Glut. */

  /** This is called if the size of the window is changed. You might
      need to update the image set-up for a different screen
      format. */
  virtual void reshape(int x, int y);

  /** This is called whenever the display needs to be redrawn. When
      called, the appropriate window has been made current. */
  virtual void display() = 0;

  /** This is called when the window is ready, for first-time
      set-up. DO NOT CALL THIS FUNCTION YOURSELF! Override this
      function, and when it is called, you can assume the gl code is
      possible. So creating viewports, GL lists, allocating textures
      etc. can be done in initGL. */
  virtual void initGL();

  /** Obtain the current widget width. */
  int getWidth();

  /** Obtain the current widget height. */
  int getHeight();

  /** Obtain current widget x-position. */
  int getXOffset();

  /** Obtain current widget y-position. */
  int getYOffset();

  /** Return a pointer to the GLWindowHelper object. Currently the only
      application of this is further querying your window, in the case
      you have a glui (glut + glui) interface. Use with care, note
      that the type of helper you get back depends on the windowing
      toolkit configured; use a dynamic_cast to verify that the
      configuration matches your assumptions. */
  inline GLWindowHelper* getOpenGLHelper() { return helper; }
};

DUECA_NS_END;

#endif

/*
  Note that for gtk2, this only works when the x-multithread-lock
  variable has been set to #t in the environment creation in
  dueca.cnf.
*/
