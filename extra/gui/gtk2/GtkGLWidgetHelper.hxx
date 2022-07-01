/* ------------------------------------------------------------------   */
/*      item            : GtkGLWidgetHelper.hxx
        made by         : Joost Ellerbroek
        date            : 100625
        category        : header file
        description     :
        changes         : 100625 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#ifndef GtkGLWidgetHelper_hxx
#define GtkGLWidgetHelper_hxx
#include <OpenGLHelper.hxx>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <dueca_ns.h>

DUECA_NS_START;

class DuecaGLCanvas;
class DuecaGLWidget;

/** This class defines the basic opengl operations. Derived classes
    implement these for specific windowing interfaces. */
class GtkGLWidgetHelper: public GLWindowHelper
{
  /// DuecaGLWidget needs access to GtkGLWidgetHelper data.
  friend class DuecaGLWidget;

protected:
  /** Drawingarea widget that will be used as gl context. */
  GtkWidget* gtk_glwidget_id;

  /** Optionally share the context between all gl windows */
  static GdkGLContext* share_context;

public:
  /** Constructor. */
  GtkGLWidgetHelper();
  /** Delete the window. */
  ~GtkGLWidgetHelper();
  /** Hide the widget. */
  void hide();
  /** Show the widget. */
  void show();
  /** Make the window's gl context current. */
  void current();
  /** Change the window's cursor type. */
  void cursor(int i);
  /** Schedule a redraw of the window. */
  void redraw();
  /** Schedule buffer swap after redraw. */
  void swap();
  /** Return the width of the GL area. */
  int width();
  /** Return the height of the GL area. */
  int height();
  /** Return the x offset of the window. */
  int xoffset();
  /** Return the y offset of the window. */
  int yoffset();
  /** Initialize drawing area as gl area. */
  void init_gl_area(DuecaGLCanvas* master);
};

/** This class provides an interface to duecaGLwindow for creation of
    windows under different windowing (toolkit) regimes. */
class GtkOpenGLHelper: public OpenGLHelper
{
public:
  /** Constructor. */
  GtkOpenGLHelper(const std::string &name);

  /** Create window. */
  GLWindowHelper* newWindow();

  /** Destructor. */
  ~GtkOpenGLHelper();
};

DUECA_NS_END;

#endif
