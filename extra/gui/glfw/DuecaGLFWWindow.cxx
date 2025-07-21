/* ------------------------------------------------------------------   */
/*      item            : DuecaGLFWWindow.cxx
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
// https://docs.gtk.org/FW/migrating-3to4.html

#include "DuecaGLFWWindow.hxx"
#include <dueca/Environment.hxx>
#include <GL/gl.h>

#include "debug.h"

DUECA_NS_START;

std::map<const GLFWwindow*, DuecaGLFWWindow*> DuecaGLFWWindow::winmap;

DuecaGLFWWindow::DuecaGLFWWindow(const char *window_title,
                                     bool pass_passive, bool depth_buffer,
                                     bool stencil_buffer) :
  glfw_win_id(NULL),
  title(window_title),
  fullscreen(false),
  depth_buffer(depth_buffer),
  stencil_buffer(stencil_buffer),
  cursortype(1),
  need_init(true),
  width(400),
  height(300),
  x(0),
  y(0),
  dopass(pass_passive)
{
  //
}

bool DuecaGLFWWindow::setFullScreen(const bool &fs)
{
  fullscreen = fs;
  return true;
}

bool DuecaGLFWWindow::setWindow(const std::vector<int> &wpos)
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

void DuecaGLFWWindow::setWindow(int posx, int posy, int width, int height)
{
  fullscreen = false;
  x = posx;
  y = posy;
  this->width = width;
  this->height = height;
}

void DuecaGLFWWindow::swapBuffers()
{
  glfwSwapBuffers(glfw_win_id);
}

static void changeCursor(int cursor, GLFWwindow *win)
{
  static GLFWcursor *crosshair = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
  static GLFWcursor *alias = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
  static GLFWcursor *pointer = glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);

  switch (cursor) {
  case 0:
    glfwSetCursor(win, NULL);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    break;
  case 1:
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    break;
  case 2:
    glfwSetCursor(win, crosshair);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    break;
  case 3:
    glfwSetCursor(win, alias);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    break;
  case 4:
    glfwSetCursor(win, pointer);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    break;
  default: // no change
    break;
  }
}

void DuecaGLFWWindow::selectCursor(int cursortype)
{
  if (glfw_win_id && this->cursortype != cursortype) {
    changeCursor(cursortype, glfw_win_id);
  }
  this->cursortype = cursortype;
}

void DuecaGLFWWindow::redraw()
{
  gtk_gl_area_queue_render(GTK_GL_AREA(area));
}

void DuecaGLFWWindow::makeCurrent()
{
  glfwMakeContextCurrent(glfw_win_id);
}

DuecaGLFWWindow::~DuecaGLFWWindow()
{
  if (glfw_win_id)
    glfwSetWindowShouldClose(glfw_win_id, 1);
}

static void on_render(GLFWwindow *win)
{
  glfwPollEvents();
  reinterpret_cast<DuecaGLFWWindow *>(glfwGetWindowUserPointer(win))->display();
}

static void on_realize(GLFWwindow *win)
{
  gtk_gl_area_make_current(area);

  auto gerr = gtk_gl_area_get_error(area);
  if (gerr) {
    /* DUECA extra.

       Unspecified error signalled by the gtk2 gl area. */
    E_XTR("Errors with the GL area " << gerr->message);
    return;
  }

  reinterpret_cast<DuecaGLFWWindow *>(self)->passShape();
  reinterpret_cast<DuecaGLFWWindow *>(self)->initGL();

  return;
}

void on_window_realize(GtkWindow *win, gpointer self)
{
  reinterpret_cast<DuecaGLFWWindow *>(self)->placeWindow();
}

void DuecaGLFWWindow::placeWindow()
{
  if (GDK_IS_X11_DISPLAY(gdk_display_id)) {

    auto surf = GDK_SURFACE(gtk_native_get_surface(GTK_NATIVE(glfw_win_id)));
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

void DuecaGLFWWindow::openWindow()
{
  glfw_win_id = glfwCreateWindow(width, height, title.c_str(), fullscreen ? glfwGetDGLFWmonitor *monitor, GLFWwindow *share)GTK_WINDOW(gtk_window_new());
  gtk_window_set_title(GTK_WINDOW(glfw_win_id), title.c_str());
  if (fullscreen) {
    gtk_window_fullscreen(glfw_win_id);
  }
  else {
    gtk_window_set_default_size(glfw_win_id, width, height);
  }

  if (!fullscreen && x >= 0 && y >= 0) {
    g_signal_connect(glfw_win_id, "realize", G_CALLBACK(on_window_realize),
                     this);
  }

  area = gtk_gl_area_new();
  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(area), depth_buffer);
  gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(area), stencil_buffer);
  gtk_widget_set_hexpand(GTK_WIDGET(area), TRUE);
  gtk_widget_set_vexpand(GTK_WIDGET(area), TRUE);

  g_signal_connect(area, "render", G_CALLBACK(on_render), this);
  g_signal_connect(area, "realize", G_CALLBACK(on_realize), this);

  // DuecaGtkInteraction::init(area);
  gtk_window_set_child(glfw_win_id, GTK_WIDGET(area));

  DuecaGtkInteraction::init(GTK_WIDGET(area));
  gtk_window_present(glfw_win_id);

  changeCursor(cursortype, GTK_WIDGET(glfw_win_id), gdk_cursor_id,
               gdk_display_id);
}

void DuecaGLFWWindow::initGL()
{
  // default implementation; noop
}

DUECA_NS_END;
