/* ------------------------------------------------------------------   */
/*      item            : DuecaGLFWWindow.cxx
        made by         : Rene van Paassen
        date            : 250722
        category        : header file
        description     :
        changes         : 250722 first version
        language        : C++
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "DuecaGLFWWindow.hxx"
#include <dueca/Environment.hxx>
#include <GL/gl.h>

#include "debug.h"

DUECA_NS_START;

DuecaGLFWWindow::DuecaGLFWWindow(const char *window_title, bool pass_passive,
                                 bool depth_buffer, bool stencil_buffer) :
  glfw_mon(NULL),
  glfw_win(NULL),
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

void DuecaGLFWWindow::swapBuffers() { }

static void changeCursor(int cursor, GLFWwindow *win)
{
  static GLFWcursor *crosshair =
    glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
  static GLFWcursor *alias = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
  static GLFWcursor *pointer =
    glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);

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
  if (glfw_win && this->cursortype != cursortype) {
    changeCursor(cursortype, glfw_win);
  }
  this->cursortype = cursortype;
}

void DuecaGLFWWindow::redraw()
{
  if (glfw_win && !glfwWindowShouldClose(glfw_win)) {
    glfwMakeContextCurrent(glfw_win);
    this->display();
    glfwSwapBuffers(glfw_win);
    glfwPollEvents();
  }
}

void DuecaGLFWWindow::makeCurrent() {  }

DuecaGLFWWindow::~DuecaGLFWWindow()
{
  if (glfw_win)
    glfwSetWindowShouldClose(glfw_win, 1);
}

static void on_render(GLFWwindow *win)
{
  glfwPollEvents();
  reinterpret_cast<DuecaGLFWWindow *>(glfwGetWindowUserPointer(win))->display();
}

static void on_realize(GLFWwindow *win)
{
  return;
}

static void on_close(GLFWWindow *win)
{

}

unsigned DuecaGLFWWindow::opened_windows = 0;

void DuecaGLFWWindow::openWindow()
{
  // first time?
  if (opened_windows == 0) {
    if (glfwInit() == GLFW_FALSE) {
      /** DUECA extra.

          Not possible to initialize GLFW, check graphics hardware.
      */
      E_XTR("Cannot initialize GLFW");
    }
  }

  // monitors
  int count;
  GLFWmonitor **monitors = glfwGetMonitors(&count);
  glfw_mon = NULL;

  // determine monitor on basis of x, y specified position
  glfw_mon = monitors[0];
  for (auto idx = count; idx--;) {
    int xpos, ypos, width, height;
    glfwGetMonitorWorkarea(monitors[idx], &xpos, &ypos, &width, &height);
    if (x >= xpos && x < xpos + width && y >= ypos && y < ypos + height) {
      glfw_mon = monitors[idx];
      break;
    }
  }

  if (x >= 0 && y >= 0 && !fullscreen) {
    glfwWindowHint(GLFW_POSITION_X, x);
    glfwWindowHint(GLFW_POSITION_Y, y);
  }
  else {
    glfwWindowHint(GLFW_POSITION_X, GLFW_ANY_POSITION);
    glfwWindowHint(GLFW_POSITION_Y, GLFW_ANY_POSITION);
  }

  glfw_win = glfwCreateWindow(width, height, title.c_str(),
                              fullscreen ? glfw_mon : NULL, NULL);
  if (!glfw_win) {
    /* DUECA extra.

       Failure to create an glfw window.
    */
    E_XTR("Failed to create glfw window.")
    return;
  }
  opened_windows++;
  glfwSetWindowUserPointer(glfw_win, this);
  glfwSetWindowCloseCallback(glfw_win, on_close);
  glfwSetWindowSizeCallback(glfw_win, on_resize);
  glfwSetWindowRefreshCallback(glfw_win, on_render);
  glfwSetKeyCallback(glfw_win, on_keypress);
  glfwSetMouseButtonCallback(glfw_win, on_mouse);
  if (dopass) {
    glfwSetCursorPosCallback(glfw_win, on_passive);
  }
  glfwMakeContextCurrent(glfw_win);
  initGL();
  changeCursor(cursortype, glfw_win);
}

void DuecaGLFWWindow::initGL()
{
  // default implementation; noop
}

DUECA_NS_END;
