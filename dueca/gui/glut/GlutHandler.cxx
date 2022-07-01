/* ------------------------------------------------------------------   */
/*      item            : GlutHandler.cxx
        made by         : Rene' van Paassen
        date            : 010322
        category        : body file
        description     :
        changes         : 010322 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define GlutHandler_cxx
#include <dueca-conf.h>
#include "debug.h"
#include "GlutHandler.hxx"
#include <Environment.hxx>
#include <iostream>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(HAVE_GL_FREEGLUT_H)
#include <GL/freeglut.h>
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#elif defined(HAVE_GLUT_H)
#include <glut.h>
#endif

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif
#include <dassert.h>


//#ifdef HAVE_SETJMP_H
//static jmp_buf env;
//#endif
DUECA_NS_START

GlutHandler::GlutHandler(const std::string& name) :
  GuiHandler(name)
{
  //
}

GlutHandler::~GlutHandler()
{
  //
}

void GlutHandler::init(bool xlib_lock)
{
#ifdef HAVE_X11_XLIB_H
  if (xlib_lock) {
    if (!XInitThreads()) {
      cerr << "Xlib thread initiation failed" << endl;
    }
  }
#endif

  static int argc = 1;
  static char* argv[] = {const_cast<char*>("dueca_run.x")};
  glutInit(&argc, argv);
  runHooks();

  // hope these options are acceptable
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA |
                      (CSE.getGraphicDepthBufferSize() ? GLUT_DEPTH : 0) |
                      (CSE.getGraphicStencilBufferSize() ? GLUT_STENCIL : 0));
}

#ifdef HAVE_GLUTMAINLOOPEVENT
void GlutHandler::passControl()
{
  assert(hooks_done);
  cout << "Low prio thread starts cyclic running with GLUT" << endl;
  in_gui = true;
  while (in_gui) {
    CSE.doLoop();
    if (glutGetWindow()) {
      glutMainLoopEvent();
    }
  }
}

void GlutHandler::returnControl()
{
  in_gui = false;
}

#else
// using traditional GLUT

static void call_environment_loop()
{
  CSE.doLoop();
}

void GlutHandler::passControl()
{
  // we do two things. While there is no window (glutGetWindow returns
  // 0), we handle timing and updates from here. As soon as a window
  // has been created, go and use the glut loop
  /* DUECA graphics.

     Information on passing control to graphics code. */
  while (glutGetWindow() == 0 && no_gui) {
    CSE.doLoop();
  }

  if (glutGetWindow() == 0) {
    // still no window, and I was stopped before making it
    return;
  }
  no_gui = false;

  // At this point, there must be a window, so I can call glutMainLoop
  glutIdleFunc(call_environment_loop);

  /* DUECA graphics.

     Information on passing control to graphics code. */
  I_SYS("Entering glut main loop forever");
  glutMainLoop();
}

void GlutHandler::returnControl()
{
  if (no_gui) {
    // have not even started GLUT yet. Just set no_gui to false, and
    // the main loop will stop
    /* DUECA graphics.

       Information on commanding end of graphics code run. */
    I_SYS("Nicely telling to stop");
    no_gui = false;
    return;
  }

  /* DUECA graphics.

     Glut cannot stop nicely, forcing dueca process stop through call
     to exit. */
  D_SYS("Glut calling exit");
  std::exit(0);
  // just hope this did not do anything terrible to the windowing code
}

#endif


DUECA_NS_END


