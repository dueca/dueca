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

#include <dueca-conf.h>
#if defined(HAVE_GL_FREEGLUT_H)
#include <GL/freeglut.h>
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#elif defined(HAVE_GLUT_H)
#include <glut.h>
#endif

#define DEBPRINTLEVEL 1
#include <debprint.h>
#include <dueca/debug.h>

DUECA_NS_START;

unsigned opened_windows = 0;

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
  DEB("New DUECAGLFWWindow");
}

bool DuecaGLFWWindow::selectGraphicsContext(bool do_select)
{
  glfwMakeContextCurrent(do_select ? glfw_win : NULL);
  return glfwGetCurrentContext() != NULL;
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

void DuecaGLFWWindow::swapBuffers() {}

static void changeCursor(int cursor, GLFWwindow *win)
{
  static GLFWcursor *crosshair =
    glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
  static GLFWcursor *alias = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4) ||                    \
  GLFW_VERSION_MAJOR > 3
  static GLFWcursor *pointer =
    glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);
#else
  static GLFWcursor *pointer = alias;
#endif
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
  DEB("Changed cursor to " << cursor);
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
  // DEB2("Redraw request and action " << reinterpret_cast<void*>(glfw_win));
  if (glfw_win && !glfwWindowShouldClose(glfw_win)) {
    glfwMakeContextCurrent(glfw_win);
    this->display();
    glfwSwapBuffers(glfw_win);
    glfwPollEvents();
    glfwMakeContextCurrent(NULL);
  }
}

void DuecaGLFWWindow::makeCurrent()
{
  // obsolete
}

DuecaGLFWWindow::~DuecaGLFWWindow()
{
  DEB("Close requested " << reinterpret_cast<void *>(glfw_win));
  if (glfw_win)
    glfwSetWindowShouldClose(glfw_win, 1);
}

static void on_render(GLFWwindow *win)
{
  DEB("Render request and action " << reinterpret_cast<void *>(win));
  auto dwin =
    reinterpret_cast<DuecaGLFWWindow *>(glfwGetWindowUserPointer(win));
  if (dwin) {
    glfwMakeContextCurrent(win);
    dwin->display();
    glfwMakeContextCurrent(NULL);
    glfwPollEvents();
  }
}

static void on_resize(GLFWwindow *win, int width, int height)
{
  DEB("Resize callback " << reinterpret_cast<void *>(win) << " " << width << ","
                         << height);
  auto dwin =
    reinterpret_cast<DuecaGLFWWindow *>(glfwGetWindowUserPointer(win));
  if (dwin) {
    dwin->newSize(width, height);
  }
}

void DuecaGLFWWindow::newSize(int w, int h)
{
  width = w;
  height = h;
  this->reshape(w, h);
}

static void on_close(GLFWwindow *win)
{
  DEB("Close callback, n windows " << opened_windows);
  assert(opened_windows);
  opened_windows--;
  if (!opened_windows) {
    glfwTerminate();
  }
}

static void on_character(GLFWwindow *win, unsigned int character)
{
  if (character <= 255) {
    auto dwin =
      reinterpret_cast<DuecaGLFWWindow *>(glfwGetWindowUserPointer(win));

    if (dwin) {
      dwin->keyboard(char(character), dwin->cur_x, dwin->cur_y);
    }
  }
}

/** A struct for translation of Gtk keycodes to the Glut key codes
    adopted in DUECA. */
struct GLFWGlutKeyLink
{
  /** Gtk keypress. */
  unsigned short glfw_key;

  /** Corresponding Glut keypress. */
  unsigned short glut_key;
};

static unsigned glut_key(unsigned keyval)
{
  static const int NGTK_TRANS_KEYS = 21;
  static const GLFWGlutKeyLink key_trans_table[NGTK_TRANS_KEYS] = {
    { GLFW_KEY_F1, GLUT_KEY_F1 },
    { GLFW_KEY_F2, GLUT_KEY_F2 },
    { GLFW_KEY_F3, GLUT_KEY_F3 },
    { GLFW_KEY_F4, GLUT_KEY_F4 },
    { GLFW_KEY_F5, GLUT_KEY_F5 },
    { GLFW_KEY_F6, GLUT_KEY_F6 },
    { GLFW_KEY_F7, GLUT_KEY_F7 },
    { GLFW_KEY_F8, GLUT_KEY_F8 },
    { GLFW_KEY_F9, GLUT_KEY_F9 },
    { GLFW_KEY_F10, GLUT_KEY_F10 },
    { GLFW_KEY_F11, GLUT_KEY_F11 },
    { GLFW_KEY_F12, GLUT_KEY_F12 },
    { GLFW_KEY_LEFT, GLUT_KEY_LEFT },
    { GLFW_KEY_UP, GLUT_KEY_UP },
    { GLFW_KEY_RIGHT, GLUT_KEY_RIGHT },
    { GLFW_KEY_DOWN, GLUT_KEY_DOWN },
    { GLFW_KEY_PAGE_UP, GLUT_KEY_PAGE_UP },
    { GLFW_KEY_PAGE_DOWN, GLUT_KEY_PAGE_DOWN },
    { GLFW_KEY_HOME, GLUT_KEY_HOME },
    { GLFW_KEY_END, GLUT_KEY_END },
    { GLFW_KEY_INSERT, GLUT_KEY_INSERT }
  };

  int ii = 0;
  while ((ii < NGTK_TRANS_KEYS) && (key_trans_table[ii].glfw_key != keyval))
    ii++;
  if (ii < NGTK_TRANS_KEYS)
    return key_trans_table[ii].glut_key;

  return 0xffffffff;
}

static void on_keypress(GLFWwindow *win, int key, int scancode, int action,
                        int mods)
{
  if (action == GLFW_RELEASE || action == GLFW_REPEAT) {
    auto dwin =
      reinterpret_cast<DuecaGLFWWindow *>(glfwGetWindowUserPointer(win));

    if (dwin) {
      auto gkey = glut_key(key);
      if (gkey != 0xffffffff) {
        dwin->special(gkey, dwin->cur_x, dwin->cur_y);
      }
    }
  }
}

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
#if (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3) ||                    \
  GLFW_VERSION_MAJOR > 3
  for (auto idx = count; idx--;) {
    int xpos, ypos, width, height;
    glfwGetMonitorWorkarea(monitors[idx], &xpos, &ypos, &width, &height);
    if (x >= xpos && x < xpos + width && y >= ypos && y < ypos + height) {
      glfw_mon = monitors[idx];
      break;
    }
  }
#endif

  DEB("Opening window");

#if (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4) ||                    \
  GLFW_VERSION_MAJOR > 3
  if (x >= 0 && y >= 0 && !fullscreen) {
    glfwWindowHint(GLFW_POSITION_X, x);
    glfwWindowHint(GLFW_POSITION_Y, y);
  }
  else {
    glfwWindowHint(GLFW_POSITION_X, GLFW_ANY_POSITION);
    glfwWindowHint(GLFW_POSITION_Y, GLFW_ANY_POSITION);
  }
#endif

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
  int w, h;
  glfwGetFramebufferSize(glfw_win, &w, &h);
  newSize(w, h);

  glfwSetWindowUserPointer(glfw_win, this);
  glfwSetWindowCloseCallback(glfw_win, on_close);
  glfwSetFramebufferSizeCallback(glfw_win, on_resize);
  glfwSetWindowRefreshCallback(glfw_win, on_render);
  glfwSetKeyCallback(glfw_win, on_keypress);
  glfwSetCharCallback(glfw_win, on_character);

  glfwSetMouseButtonCallback(
    glfw_win, [](GLFWwindow *win, int button, int action, int mods) {
      auto dwin =
        reinterpret_cast<DuecaGLFWWindow *>(glfwGetWindowUserPointer(win));
      DEB("Mouse click")
      dwin->mouse(button, action == GLFW_PRESS ? GLUT_DOWN : GLUT_UP,
                  dwin->cur_x, dwin->cur_y);
    });

  glfwSetCursorPosCallback(glfw_win, [](GLFWwindow *win, double x, double y) {
    auto dwin =
      reinterpret_cast<DuecaGLFWWindow *>(glfwGetWindowUserPointer(win));
    DEB1("Cursor motion " << x << "," << y);
    dwin->cur_x = int(x);
    dwin->cur_y = int(y);
    if (dwin->dopass) {
      dwin->motion(dwin->cur_x, dwin->cur_y);
    }
  });

  // initialize GL code
  glfwMakeContextCurrent(glfw_win);
  initGL();
  glfwMakeContextCurrent(NULL);

  // set desired cursor
  changeCursor(cursortype, glfw_win);

  // force draw and set visible
  glfwShowWindow(glfw_win);
}

// NOOP implementations
void DuecaGLFWWindow::initGL() {}

void DuecaGLFWWindow::reshape(int x, int y) {}

void DuecaGLFWWindow::keyboard(char c, int x, int y) {}

void DuecaGLFWWindow::special(int c, int x, int y) {}

void DuecaGLFWWindow::motion(int x, int y) {}

void DuecaGLFWWindow::passive(int x, int y) {}

void DuecaGLFWWindow::mouse(int button, int state, int x, int y) {}

DUECA_NS_END;
