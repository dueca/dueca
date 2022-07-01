/* ------------------------------------------------------------------   */
/*      item            : GtkGLWidgetHelper.cxx
        made by         : Joost Ellerbroek
        date            : 100625
        category        : body file
        description     :
        changes         : 100625 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#include "GtkOpenGLHelper.hxx"
#include "DuecaGLCanvas.hxx"
#include <GL/glut.h>
#include <gdk/gdkkeysyms.h>
#include <Environment.hxx>
#include <dassert.h>
#include <GuiHandler.hxx>

DUECA_NS_START;

GdkGLContext* GtkGLWidgetHelper::share_context = NULL;

// gl configuration
static GdkGLConfig* glconfig = NULL;

inline void redraw_done(DuecaGLCanvas* gw)
{
  gw->redrawDone();
}

static void on_gtk_gl_realize(GtkWidget *w, gpointer user_data)
{
  GTK_DRAWING_AREA(w);
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (w);
  GdkGLContext *glcontext = gtk_widget_get_gl_context (w);
  if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext)) return;
  DuecaGLCanvas* gw = reinterpret_cast<DuecaGLCanvas*>(user_data);
  gw->initGL();
  gdk_gl_drawable_gl_end (gldrawable);
}

static gboolean on_gtk_gl_configure_event(GtkWidget *w,
                                          GdkEventConfigure *event,
                                          gpointer user_data)
{
  GdkGLContext *glcontext = gtk_widget_get_gl_context (w);
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (w);
  if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext)) return FALSE;
  DuecaGLCanvas* gw = reinterpret_cast<DuecaGLCanvas*>(user_data);
  gw->reshape(event->width, event->height);
  gdk_gl_drawable_gl_end (gldrawable);
  return TRUE;
}

static gboolean on_gtk_gl_expose_event(GtkWidget *w,
                                       GdkEventExpose *event,
                                       gpointer user_data)
{
  GdkGLContext *glcontext = gtk_widget_get_gl_context (w);
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (w);
  if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext)) return FALSE;
  DuecaGLCanvas* gw = reinterpret_cast<DuecaGLCanvas*>(user_data);
  gw->display();
  redraw_done(gw);
  gdk_gl_drawable_gl_end (gldrawable);
  return TRUE;
}

GtkGLWidgetHelper::GtkGLWidgetHelper() :
    gtk_glwidget_id(NULL)
{
  //
}

GtkGLWidgetHelper::~GtkGLWidgetHelper()
{
  //
}

void GtkGLWidgetHelper::hide()
{
  gtk_widget_hide_all(gtk_glwidget_id);
}

void GtkGLWidgetHelper::show()
{
  gtk_widget_show_all(gtk_glwidget_id);
}

void GtkGLWidgetHelper::current()
{
  GdkGLContext *glcontext = gtk_widget_get_gl_context (gtk_glwidget_id);
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (gtk_glwidget_id);
  gdk_gl_drawable_make_current(gldrawable, glcontext);
}

void GtkGLWidgetHelper::cursor(int i)
{
  // not implemented
}

void GtkGLWidgetHelper::redraw()
{
  GdkRectangle area;
  area.x = 0; area.y = 0;
  area.width = gtk_glwidget_id->allocation.width;
  area.height = gtk_glwidget_id->allocation.height;
  gtk_widget_draw(gtk_glwidget_id, &area);
}

void GtkGLWidgetHelper::swap()
{
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (gtk_glwidget_id);
  if (gdk_gl_drawable_is_double_buffered (gldrawable)) {
    GdkGLContext *glcontext = gtk_widget_get_gl_context (gtk_glwidget_id);
    if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext)) return;
    gdk_gl_drawable_swap_buffers (gldrawable);
    gdk_gl_drawable_gl_end (gldrawable);
  } else {
    glFlush();
  }
}

int GtkGLWidgetHelper::width()
{
  return gtk_glwidget_id->allocation.width;
}

int GtkGLWidgetHelper::height()
{
  return gtk_glwidget_id->allocation.height;
}

int GtkGLWidgetHelper::xoffset()
{
  gint x = 0;
  GdkWindow* gdk_tl_window = gdk_window_get_toplevel(gtk_glwidget_id->window);
  gdk_window_get_origin(gdk_tl_window, &x, NULL);
  return x;
}

int GtkGLWidgetHelper::yoffset()
{
  gint y = 0;
  GdkWindow* gdk_tl_window = gdk_window_get_toplevel(gtk_glwidget_id->window);
  gdk_window_get_origin(gdk_tl_window, NULL, &y);
  return y;
}

void GtkGLWidgetHelper::init_gl_area(DuecaGLCanvas* master)
{
  if (!glconfig) {
    cerr << "No opengl configuration" << endl;
    exit(1);
  }

  // add opengl capability to the widget
  gboolean res = gtk_widget_set_gl_capability
      (gtk_glwidget_id, glconfig, share_context, TRUE, GDK_GL_RGBA_TYPE);
  if (!res) {
    cerr << "Cannot set openglcapability" << endl;
    exit(1);
  }

  gtk_widget_add_events(gtk_glwidget_id, GDK_POINTER_MOTION_MASK |
          GDK_POINTER_MOTION_HINT_MASK |
          GDK_BUTTON_MOTION_MASK |
          GDK_BUTTON_PRESS_MASK |
          GDK_BUTTON_RELEASE_MASK |
          GDK_KEY_PRESS_MASK |
          GDK_SCROLL_MASK);

  g_signal_connect_after (G_OBJECT (gtk_glwidget_id), "realize",
                          G_CALLBACK (on_gtk_gl_realize),
                                      master);
  g_signal_connect (G_OBJECT (gtk_glwidget_id), "configure-event",
                    G_CALLBACK (on_gtk_gl_configure_event),
                                master);
  g_signal_connect (G_OBJECT (gtk_glwidget_id), "expose-event",
                    G_CALLBACK (on_gtk_gl_expose_event),
                                master);


  GTK_WIDGET_SET_FLAGS(gtk_glwidget_id, GTK_CAN_FOCUS);
  gtk_widget_grab_focus(gtk_glwidget_id);
  gtk_widget_add_events(gtk_glwidget_id, GDK_EXPOSURE_MASK);

  gtk_widget_realize(gtk_glwidget_id);
  gtk_widget_show(gtk_glwidget_id);

  if (!share_context && CSE.getShareGLContexts())
    share_context = gtk_widget_get_gl_context (gtk_glwidget_id);
}

extern int* p_argc;
extern char*** p_argv;

static void initGtkGLext(const std::string& selected)
{
  // check that the right gui has been selected
  if (selected != std::string("gtk2")) return;

  if (!glconfig) {
    gtk_gl_init (p_argc, p_argv);

    int attrlist[] =
    { GDK_GL_RGBA, 1,
      GDK_GL_DOUBLEBUFFER, 1,
      GDK_GL_DEPTH_SIZE, CSE.getGraphicDepthBufferSize(),
      GDK_GL_STENCIL_SIZE, CSE.getGraphicStencilBufferSize(),
      GDK_GL_ATTRIB_LIST_NONE};
    glconfig = gdk_gl_config_new(attrlist);

    if (glconfig == NULL) {
      cerr << "Cannot open double buffer GL visual, trying single" << endl;
      int attrlist[] =
      { GDK_GL_RGBA,
        GDK_GL_DEPTH_SIZE, CSE.getGraphicDepthBufferSize(),
        GDK_GL_ATTRIB_LIST_NONE};
      glconfig = gdk_gl_config_new(attrlist);
      if (glconfig == NULL) {
        cerr << "No OpenGL visuals" << endl;
        exit(1);
      }
    }
  }
}

GtkOpenGLHelper::GtkOpenGLHelper(const std::string &name) :
    OpenGLHelper(name)
{
  // add gl configuration to init
  GuiHandler::addInitHook(initGtkGLext);
}

GtkOpenGLHelper::~GtkOpenGLHelper()
{
  //
}

GLWindowHelper* GtkOpenGLHelper::newWindow()
{
  return new GtkGLWindowHelper();
}

DUECA_NS_END;
