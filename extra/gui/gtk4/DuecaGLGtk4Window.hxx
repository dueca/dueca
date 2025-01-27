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
#include <gtk/gtk.h>
#include <dueca_ns.h>
#include <string>
#include <vector>

#include "DuecaGtkInteraction.hxx"

DUECA_NS_START;

/** Provides a DUECA shell around a window with a Gtk4 GtkGlarea.

    After deriving from this class, you should implement
    the GL drawing routine.

    You can also implement callbacks for reshape, initialisation and
    mouse and keyboard events, check DuecaGtkInteraction for these.

    Note that these GL windows are primarily for testing. Using e.g.
    BareDuecaGLWindow you will get a window with lower overhead, that can
    also be run in a separate thread, blocking for the graphics refresh.
    Use that for serious deployment, and the GTK version for testing.
*/
class DuecaGLGtk4Window : public DuecaGtkInteraction
{
  /** Display */
  GdkDisplay *gdk_display_id;

  /** Window */
  GtkWindow *gtk_win_id;

  /** pointer to the underlying GL area */
  GtkWidget *area;

  /** cursor to be used */
  GdkCursor *gdk_cursor_id;

  /** Window title */
  std::string title;

  /** fullscreen? */
  bool fullscreen;

  /** selected cursor */
  int cursortype;

  /** did the init */
  bool need_init;

public:
  /** Constructor

      @param window_title Title for the window
      @param pass_passive For compatibility, passive movement always passed.
  */
  DuecaGLGtk4Window(const char *window_title = "DUECA",
                    bool pass_passive = false);

  /** Backwards compatible constructor. Arguments, except
      mouse_passive, are ignored. */
  DuecaGLGtk4Window(const char *window_title, bool dummy1, bool dummy2,
                    bool dummy3 = true, bool dummy4 = true,
                    bool mouse_passive = true);

  /// Destructor
  ~DuecaGLGtk4Window();

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
};

DUECA_NS_END;
