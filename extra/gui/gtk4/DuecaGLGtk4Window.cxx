/* ------------------------------------------------------------------   */
/*      item            : DuecaGLGtk4Window.cxx
        made by         : Rene van Paassen
        date            : 180607
        category        : header file
        description     :
        changes         : 180607 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/
// https://docs.gtk.org/gtk4/migrating-3to4.html

#include "DuecaGLGtk4Window.hxx"
#include <dueca/Environment.hxx>
#include <GL/gl.h>
#include <gtk/gtk.h>

#include "debug.h"
#include "gdk/gdk.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#endif

DUECA_NS_START;

DuecaGLGtk4Window::DuecaGLGtk4Window(const char *window_title,
                                     bool pass_passive) :
  DuecaGtkInteraction(NULL, 400, 300),
  gdk_display_id(NULL),
  gtk_win_id(NULL),
  area(NULL),
  gdk_cursor_id(NULL),
  title(window_title),
  fullscreen(false),
  cursortype(1)
{
  //
}

DuecaGLGtk4Window::DuecaGLGtk4Window(const char *window_title, bool dummy1,
                                     bool dummy2, bool dummy3, bool dummy4,
                                     bool pass_passive) :
  DuecaGtkInteraction(NULL, 400, 300),
  gdk_display_id(NULL),
  gtk_win_id(NULL),
  area(NULL),
  gdk_cursor_id(NULL),
  title(window_title),
  fullscreen(false),
  cursortype(1)
{
  //
}

bool DuecaGLGtk4Window::setFullScreen(const bool &fs)
{
  fullscreen = fs;
  return true;
}

bool DuecaGLGtk4Window::setWindow(const std::vector<int> &wpos)
{
  if (wpos.size() == 2) {
    x = wpos[0];
    y = wpos[1];
    return true;
  }
  if (wpos.size() == 4) {
    setWindow(wpos[0], wpos[1], wpos[2], wpos[3]);
    return true;
  }
  return false;
}

void DuecaGLGtk4Window::setWindow(int posx, int posy, int width, int height)
{
  fullscreen = false;
  x = posx;
  y = posy;
  this->width = width;
  this->height = height;
}

void DuecaGLGtk4Window::swapBuffers()
{
  glFlush();
  // no-op?
}

static void changeCursor(int cursor, GtkWidget *win, GdkCursor *&gcursor,
                         GdkDisplay *display)
{
  if (gcursor) {
    g_object_unref(G_OBJECT(gcursor));
  }
  gcursor = NULL;
  switch (cursor) {
  case 0:
    gcursor = gdk_cursor_new_from_name("default", NULL);
    break;
  case 1:
    gcursor = gdk_cursor_new_from_name("none", NULL);
    break;
  case 2:
    gcursor = gdk_cursor_new_from_name("crosshair", NULL);
    break;
  case 3:
    gcursor = gdk_cursor_new_from_name("alias", NULL);
    break;
  case 4:
    gcursor = gdk_cursor_new_from_name("pointer", NULL);
    break;
  default: // no change
    break;
  }
  gtk_widget_set_cursor(win, gcursor);
}

void DuecaGLGtk4Window::selectCursor(int cursortype)
{
  if (gtk_win_id && this->cursortype != cursortype) {
    changeCursor(cursortype, GTK_WIDGET(gtk_win_id), gdk_cursor_id,
                 gdk_display_id);
  }
  this->cursortype = cursortype;
}

void DuecaGLGtk4Window::redraw()
{
  gtk_gl_area_queue_render(GTK_GL_AREA(area));
}

void DuecaGLGtk4Window::makeCurrent()
{
  gtk_gl_area_make_current(GTK_GL_AREA(area));
}

DuecaGLGtk4Window::~DuecaGLGtk4Window()
{
  if (gdk_cursor_id)
    g_object_unref(G_OBJECT(gdk_cursor_id));
  if (gtk_win_id)
    g_object_unref(G_OBJECT(gtk_win_id));
}

static gboolean on_render(GtkGLArea *area, GdkGLContext *context, gpointer self)
{
  // if (gtk_gl_area_get_error(area) != NULL)
  //   return FALSE;

  // gtk_gl_area_make_current(area);

  reinterpret_cast<DuecaGLGtk4Window *>(self)->display();
  return TRUE;
}

static void on_realize(GtkGLArea *area, gpointer self)
{
  gtk_gl_area_make_current(area);
  gtk_gl_area_attach_buffers(area);

  auto gerr = gtk_gl_area_get_error(area);
  if (gerr) {
    /* DUECA extra.

       Unspecified error signalled by the gtk2 gl area. */
    E_XTR("Errors with the GL area " << gerr->message);
    return;
  }

  reinterpret_cast<DuecaGLGtk4Window *>(self)->passShape();
  reinterpret_cast<DuecaGLGtk4Window *>(self)->initGL();

  return;
}

void on_window_realize(GtkWindow *win, gpointer self)
{
  reinterpret_cast<DuecaGLGtk4Window *>(self)->placeWindow();
}

void DuecaGLGtk4Window::placeWindow()
{
  if (GDK_IS_X11_DISPLAY(gdk_display_id)) {

    auto surf = GDK_SURFACE(gtk_native_get_surface(GTK_NATIVE(gtk_win_id)));
    if (surf) {
      auto xw = GDK_SURFACE_XID(surf);
      auto xd = GDK_SURFACE_XDISPLAY(surf);
      if (xd) {
        XMoveWindow(xd, xw, x, y);
      }
    }
  }
  else if (GDK_IS_WAYLAND_DISPLAY(gdk_display_id)) {
    /* DUECA extra.

          Under wayland, it is (currently) not possible to request a window
          position
        */
    W_XTR("Cannot influence window position on wayland");
  }
}

void DuecaGLGtk4Window::openWindow()
{
  gdk_display_id = gdk_display_get_default();
  gtk_win_id = GTK_WINDOW(gtk_window_new());
  gtk_window_set_title(GTK_WINDOW(gtk_win_id), title.c_str());

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
  gtk_window_set_child(GTK_WINDOW(gtk_win_id), box);
  if (!fullscreen && x >= 0 && y >= 0) {
    g_signal_connect(gtk_win_id, "realize", G_CALLBACK(on_window_realize),
                     this);
  }
  area = gtk_gl_area_new();
  gtk_gl_area_set_required_version(GTK_GL_AREA(area), 4, 4);
  gtk_gl_area_set_auto_render(GTK_GL_AREA(area), FALSE);
  if (CSE.getGraphicDepthBufferSize()) {
    gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(area), TRUE);
  }
  if (CSE.getGraphicStencilBufferSize()) {
    gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(area), TRUE);
  }

  if (fullscreen) {
    gtk_window_fullscreen(gtk_win_id);
  }
  else if (width > 0 && height > 0) {
    gtk_widget_set_size_request(area, width, height);
  }

  g_signal_connect(area, "render", G_CALLBACK(on_render), this);
  g_signal_connect(area, "realize", G_CALLBACK(on_realize), this);

  // DuecaGtkInteraction::init(area);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(area));

  DuecaGtkInteraction::init(GTK_WIDGET(area));

  gtk_widget_set_visible(GTK_WIDGET(gtk_win_id), TRUE);
  changeCursor(cursortype, GTK_WIDGET(gtk_win_id), gdk_cursor_id,
               gdk_display_id);
}

void DuecaGLGtk4Window::initGL()
{
  // default implementation; noop
}

DUECA_NS_END;
