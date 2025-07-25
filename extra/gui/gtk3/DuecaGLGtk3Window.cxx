/* ------------------------------------------------------------------   */
/*      item            : DuecaGLGtk3Window.cxx
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

#include "DuecaGLGtk3Window.hxx"
#include <dueca/Environment.hxx>

#include "debug.h"

DUECA_NS_START;

DuecaGLGtk3Window::DuecaGLGtk3Window(const char *window_title,
                                     bool pass_passive, bool depth_buffer,
                                     bool stencil_buffer) :
  DuecaGtkInteraction(NULL, 400, 300),
  gdk_display_id(NULL),
  gtk_win_id(NULL),
  area(NULL),
  gdk_cursor_id(NULL),
  title(window_title),
  fullscreen(false),
  depth_buffer(depth_buffer ? TRUE : FALSE),
  stencil_buffer(stencil_buffer ? TRUE : FALSE),
  cursortype(1)
{
  //
}

bool DuecaGLGtk3Window::selectGraphicsContext(bool do_select)
{
  if (do_select) {
    gtk_gl_area_make_current(GTK_GL_AREA(area));
  }
  return do_select;
}

bool DuecaGLGtk3Window::setFullScreen(const bool &fs)
{
  fullscreen = fs;
  return true;
}

bool DuecaGLGtk3Window::setWindow(const std::vector<int> &wpos)
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

void DuecaGLGtk3Window::setWindow(int posx, int posy, int width, int height)
{
  fullscreen = false;
  x = posx;
  y = posy;
  this->width = width;
  this->height = height;
}

void DuecaGLGtk3Window::swapBuffers()
{
  // glFlush();
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
    gcursor = gdk_cursor_new_from_name(display, "default");
    break;
  case 1:
    assert(gcursor == NULL);
    break;
  case 2:
    gcursor = gdk_cursor_new_from_name(display, "crosshair");
    break;
  case 3:
    gcursor = gdk_cursor_new_for_display(display, GDK_ARROW);
    break;
  case 4:
    gcursor = gdk_cursor_new_for_display(display, GDK_LEFT_PTR);
    break;
  default: // no change
    break;
  }
}

void DuecaGLGtk3Window::selectCursor(int cursortype)
{
  if (gtk_win_id && this->cursortype != cursortype) {
    changeCursor(cursortype, GTK_WIDGET(gtk_win_id), gdk_cursor_id,
                 gdk_display_id);
  }
  this->cursortype = cursortype;
}

void DuecaGLGtk3Window::redraw()
{
  gtk_gl_area_queue_render(GTK_GL_AREA(area));
}

void DuecaGLGtk3Window::makeCurrent()
{
  gtk_gl_area_make_current(GTK_GL_AREA(area));
}

DuecaGLGtk3Window::~DuecaGLGtk3Window()
{
  gtk_gl_area_make_current(GTK_GL_AREA(area));
  if (gdk_cursor_id)
    g_object_unref(G_OBJECT(gdk_cursor_id));
  if (gtk_win_id)
    g_object_unref(G_OBJECT(gtk_win_id));
}

static gboolean on_render(GtkGLArea *area, GdkGLContext *context, gpointer self)
{
  if (gtk_gl_area_get_error(area) != NULL)
    return FALSE;

  reinterpret_cast<DuecaGLGtk3Window *>(self)->display();
  return TRUE;
}

GdkGLContext *DUECA_GTK3GL_common_gc = NULL;
bool DUECA_GTK3GL_common_gc_created = false;

static void on_realize(GtkGLArea *area, gpointer self)
{
  gtk_gl_area_make_current(area);

  auto gerr = gtk_gl_area_get_error(area);
  if (gerr) {
    /* DUECA extra.

       Unspecified error signalled by the gtk2 gl area. */
    E_XTR("Errors with the GL area " << gerr->message);
    return;
  }

  // reinterpret_cast<DuecaGLGtk3Window*>(self)->initGL();
  if (DUECA_GTK3GL_common_gc == NULL) {
    DUECA_GTK3GL_common_gc = gtk_gl_area_get_context(area);
  }
  reinterpret_cast<DuecaGLGtk3Window *>(self)->passShape();
  reinterpret_cast<DuecaGLGtk3Window *>(self)->initGL();

  return;
}

static GdkGLContext *on_context(GtkGLArea *area, gpointer self)
{
  assert(DUECA_GTK3GL_common_gc != NULL);
  return gdk_gl_context_get_shared_context(DUECA_GTK3GL_common_gc);
}

void DuecaGLGtk3Window::openWindow()
{
  gdk_display_id = gdk_display_get_default();
  gtk_win_id = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  g_object_ref(G_OBJECT(gtk_win_id));
  gtk_window_set_title(GTK_WINDOW(gtk_win_id), title.c_str());

  area = gtk_gl_area_new();
  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(area), depth_buffer);
  gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(area), stencil_buffer);
  gtk_widget_set_hexpand(GTK_WIDGET(area), TRUE);
  gtk_widget_set_vexpand(GTK_WIDGET(area), TRUE);

  // gtk_gl_area_set_required_version(GTK_GL_AREA(area), 4, 0);
  // gtk_gl_area_set_auto_render(GTK_GL_AREA(area), FALSE);
  gtk_gl_area_set_has_alpha(GTK_GL_AREA(area), TRUE);

  if (fullscreen) {
    gtk_window_fullscreen(gtk_win_id);
  }
  else if (width > 0 && height > 0) {
    gtk_widget_set_size_request(area, width, height);
  }

  g_signal_connect(area, "render", G_CALLBACK(on_render), this);
  g_signal_connect(area, "realize", G_CALLBACK(on_realize), this);
  if (DUECA_GTK3GL_common_gc_created) {
    g_signal_connect(area, "create-context", G_CALLBACK(on_context), this);
  }
  else if (CSE.getShareGLContexts()) {
    DUECA_GTK3GL_common_gc_created = true;
  }

  // DuecaGtkInteraction::init(area);
  gtk_container_add(GTK_CONTAINER(gtk_win_id), GTK_WIDGET(area));

  DuecaGtkInteraction::init(GTK_WIDGET(area));

  gtk_widget_show_all(GTK_WIDGET(gtk_win_id));
  if (!fullscreen && x >= 0 && y >= 0) {
    gtk_window_move(gtk_win_id, x, y);
  }

  changeCursor(cursortype, GTK_WIDGET(gtk_win_id), gdk_cursor_id,
               gdk_display_id);
}

void DuecaGLGtk3Window::initGL()
{
  // default implementation; noop
}

DUECA_NS_END;
