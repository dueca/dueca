/* ------------------------------------------------------------------   */
/*      item            : GlutOpenGLHelper.cxx
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


#define GlutOpenGLHelper_cxx
#include <dueca-conf.h>

#if defined(HAVE_GL_FREEGLUT_H)
#include <GL/freeglut.h>
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#elif defined(HAVE_GLUT_H)
#include <glut.h>
#endif

#include <dueca_ns.h>

#include "GlutOpenGLHelper.hxx"
#include "DuecaGLWindow.hxx"
#include "GLSweeper.hxx"

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
  object_map[glutGetWindow()]->keyboard(key, x, y);
}

static void special(int key, int x, int y)
{
  object_map[glutGetWindow()]->special(key, x, y);
}

static void display(void)
{
  DEB("display win "  << glutGetWindow());
  DuecaGLWindow *master = object_map[glutGetWindow()];
  if (check_and_mark_init_done(master)) {
    master->initGL();
  }
  master->display(); CHECKGL;
  redraw_done(master);
}

static void reshape(int x, int y)
{
  DEB("reshape win " << glutGetWindow());

  DuecaGLWindow *master = object_map[glutGetWindow()];
  if (check_and_mark_init_done(master)) {
    master->initGL();
  }
  master->reshape(x, y);
}

static void mouse(int button, int state, int x, int y)
{
  object_map[glutGetWindow()]->mouse(button, state, x, y);
}

static void motion(int x, int y)
{
  object_map[glutGetWindow()]->motion(x, y);
}

static void passive(int x, int y)
{
  object_map[glutGetWindow()]->passive(x, y);
}

GlutGLWindowHelper::GlutGLWindowHelper() :
  glut_win_id(-1)
{
  //
}

void GlutGLWindowHelper::open(const std::string &title,
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
  DEB("created win " << glut_win_id);
  //glutSetCursor(GLUT_CURSOR_NONE);
  object_map[glut_win_id] = master;

  // make fullscreen if needed
  if (fullscreen) glutFullScreen();

  // install callback functions. Do NOT install idle func, since that
  // would royally mess up DUECA
  bool install_functions = true;
  if (install_functions) {
    glutKeyboardFunc(keyboard); CHECKGL;
    glutSpecialFunc(special); CHECKGL;
    glutMouseFunc(mouse); CHECKGL;
    glutMotionFunc(motion); CHECKGL;
    glutPassiveMotionFunc(passive); CHECKGL;
    glutDisplayFunc(display); CHECKGL;
    glutReshapeFunc(reshape); CHECKGL;
    install_functions = false;
  }
}

GlutGLWindowHelper::~GlutGLWindowHelper()
{
  //
}

void GlutGLWindowHelper::hide()
{
  current();
  glutHideWindow();
}


void GlutGLWindowHelper::show()
{
  current();
  glutShowWindow();
}


void GlutGLWindowHelper::current()
{
  glutSetWindow(glut_win_id);
}


void GlutGLWindowHelper::cursor(int i)
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


void GlutGLWindowHelper::redraw()
{
  current();
  glutPostRedisplay();
}


void GlutGLWindowHelper::swap()
{
  current();
  glutSwapBuffers();
}


int GlutGLWindowHelper::width()
{
  current();
  return glutGet(GLUT_WINDOW_WIDTH);
}


int GlutGLWindowHelper::height()
{
  current();
  return glutGet(GLUT_WINDOW_HEIGHT);
}

int GlutGLWindowHelper::xoffset()
{
  current();
  return glutGet(GLUT_WINDOW_X);
}

int GlutGLWindowHelper::yoffset()
{
  current();
  return glutGet(GLUT_WINDOW_Y);
}

GlutOpenGLHelper::GlutOpenGLHelper(const std::string &name) :
  OpenGLHelper(name)
{
  //
}

GlutOpenGLHelper::~GlutOpenGLHelper()
{
  //
}


GLWindowHelper* GlutOpenGLHelper::newWindow()
{
  return new GlutGLWindowHelper();
}

DUECA_NS_END;

