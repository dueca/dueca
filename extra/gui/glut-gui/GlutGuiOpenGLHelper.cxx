/* ------------------------------------------------------------------   */
/*      item            : GlutGuiOpenGLHelper.cxx
        made by         : Rene' van Paassen
        date            : 060531
        category        : body file
        description     :
        changes         : 060531 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define GlutGuiOpenGLHelper_cxx

#include <dueca-conf.h>
#include <dueca_ns.h>

#include "GlutGuiOpenGLHelper.hxx"
#include "DuecaGLWindow.hxx"
#include "GLSweeper.hxx"

#if defined(HAVE_GL_FREEGLUT_H)
#include <GL/freeglut.h>
#define GLUI_FREEGLUT
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#elif defined(HAVE_GLUT_H)
#include <glut.h>
#endif
#if defined(HAVE_GL_GLUI_H)
#include <GL/glui.h>
#elif defined(HAVE_GLUI_H)
#include <glui.h>
#else
#error "Have not found glui.h"
#endif
#include <Environment.hxx>

#ifdef TEST_OPTIONS
#define CHECKGL \
  { GLint err = glGetError(); \
    if (err != GL_NO_ERROR) { \
      cerr << __FILE__ "/" << __LINE__ << " GL error " << err << endl; \
    } \
  }
//#define DEB(A) cerr << A << endl;
#define DEB(A)
#else
#define CHECKGL
#define DEB(A)
#endif


DUECA_NS_START;

inline void redraw_done(DuecaGLCanvas* gw)
{
  gw->redrawDone();
}

inline bool check_and_mark_init_done(DuecaGLCanvas* gw)
{
  return gw->checkAndMarkInitDone();
}

static map<int,DuecaGLWindow*> object_map;

static void keyboard(unsigned char key, int x, int y)
{
  DuecaGLWindow *master = object_map[glutGetWindow()];
  if (master) {
    master->keyboard(key, x, y);
  }
  //glui_keyboard_func(key, x, y);
}

static void special(int key, int x, int y)
{
  DuecaGLWindow *master = object_map[glutGetWindow()];
  if (master) {
    master->special(key, x, y);
  }
  //glui_special_func(key, x, y);
}

static void display(void)
{
  DEB("display win " << glutGetWindow());
  DuecaGLWindow *master = object_map[glutGetWindow()];
  if (master) {
    if (check_and_mark_init_done(master)) {
      master->initGL();
    }
    master->display();  CHECKGL;
    redraw_done(master);
  }
}

static void mouse(int button, int state, int x, int y)
{
  DEB("mouse, b=" << button << " s=" << state << " x=" << x << " y=" << y);
  DuecaGLWindow *master = object_map[glutGetWindow()];
  if (master) {
    master->mouse(button, state, x, y);
  }
  //glui_parent_window_mouse_func(button, state, x, y);
}

static void motion(int x, int y)
{
  DuecaGLWindow *master = object_map[glutGetWindow()];
  if (master) {
    master->motion(x, y);
  }
}

static void passive(int x, int y)
{
  DuecaGLWindow *master = object_map[glutGetWindow()];
  if (master) {
    master->passive(x, y);
  }
}


static void reshape(int x, int y)
{
  DEB("reshape win " << glutGetWindow());

  DuecaGLWindow *master = object_map[glutGetWindow()];
  if (master) {
    if (check_and_mark_init_done(master)) {
      master->initGL();
    }
    master->reshape(x, y);
  }
}




GlutGuiGLWindowHelper::GlutGuiGLWindowHelper() :
  glut_win_id(-1)
{
  //
}

void GlutGuiGLWindowHelper::open(const std::string &title,
                                 DuecaGLWindow* master,
                                 int offset_x, int offset_y,
                                 int size_x, int size_y,
                                 bool fullscreen)
{
  glutInitWindowSize(size_x, size_y);
  if (offset_x != -1 && offset_y != -1) {
    glutInitWindowPosition(offset_x, offset_y);
  }

  unsigned int glut_flags = 0;
  if (GLSweeper_single()) {
    if (GLSweeper_single()->getDoubleBuffer()) {
      glut_flags |= GLUT_DOUBLE;
    }
    if (GLSweeper_single()->getAlphaBuffer()) {
      glut_flags |= GLUT_RGBA | GLUT_ALPHA;
    }
    else {
      glut_flags |= GLUT_RGB;
    }
    if (GLSweeper_single()->getDepthBufferSize()) {
      glut_flags |= GLUT_DEPTH;
    }
    if (GLSweeper_single()->getStencilBufferSize()) {
      glut_flags |= GLUT_STENCIL;
    }
  }
  else {
    glut_flags = GLUT_DOUBLE | GLUT_RGBA;
    if (CSE.getGraphicDepthBufferSize()) glut_flags |= GLUT_DEPTH;
    if (CSE.getGraphicStencilBufferSize()) glut_flags |= GLUT_STENCIL;
  }
  glutInitDisplayMode(glut_flags);

  // create window
  glut_win_id = glutCreateWindow(title.c_str());

  // create a glui sub-window, connect the links
  //glui = GLUI_Master.create_glui_subwindow(glut_win_id, 0);
  //GLUI_Master.create_glui
  //glut_win_id = glui->get_glut_window_id();
  object_map[glut_win_id] = master;
  DEB("created win " << glut_win_id);

  // make fullscreen if needed
  if (fullscreen) glutFullScreen();

  // install callback functions. Do NOT install idle func, since that
  // would royally mess up DUECA
  GLUI_Master.set_glutKeyboardFunc(keyboard);
  GLUI_Master.set_glutSpecialFunc(special);
  GLUI_Master.set_glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutPassiveMotionFunc(passive);
  glutDisplayFunc(display);
  GLUI_Master.set_glutReshapeFunc(reshape);
  GLUI_Master.set_glutIdleFunc(NULL);
}

GlutGuiGLWindowHelper::~GlutGuiGLWindowHelper()
{
  //
}

void GlutGuiGLWindowHelper::hide()
{
  current();
  glutHideWindow();
}


void GlutGuiGLWindowHelper::show()
{
  current();
  glutShowWindow();
}


void GlutGuiGLWindowHelper::current()
{
  glutSetWindow(glut_win_id);
}


void GlutGuiGLWindowHelper::cursor(int i)
{
  current();

  switch(i) {
  case 0:
    glutSetCursor(GLUT_CURSOR_NONE);
    break;
  case 1:
    glutSetCursor(GLUT_CURSOR_INHERIT);
    break;
  case 2:
    glutSetCursor(GLUT_CURSOR_CROSSHAIR);
    break;
  case 3:
    glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
    break;
  case 4:
    glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
    break;
  }
}


void GlutGuiGLWindowHelper::redraw()
{
  current();
  glutPostRedisplay();
}


void GlutGuiGLWindowHelper::swap()
{
  current();
  glutSwapBuffers();
}


int GlutGuiGLWindowHelper::width()
{
  current();
  return glutGet(GLUT_WINDOW_WIDTH);
}


int GlutGuiGLWindowHelper::height()
{
  current();
  return glutGet(GLUT_WINDOW_HEIGHT);
}

int GlutGuiGLWindowHelper::xoffset()
{
  current();
  return glutGet(GLUT_WINDOW_X);
}

int GlutGuiGLWindowHelper::yoffset()
{
  current();
  return glutGet(GLUT_WINDOW_Y);
}

GlutGuiOpenGLHelper::GlutGuiOpenGLHelper(const std::string &name) :
  OpenGLHelper(name)
{
  //
}

GlutGuiOpenGLHelper::~GlutGuiOpenGLHelper()
{
  //
}


GLWindowHelper* GlutGuiOpenGLHelper::newWindow()
{
  return new GlutGuiGLWindowHelper();
}

DUECA_NS_END;

