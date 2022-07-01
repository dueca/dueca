/* ------------------------------------------------------------------   */
/*      item            : GluiHandler.cxx
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
#include "GluiHandler.hxx"
#include <Environment.hxx>
#include <iostream>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
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
#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif
#include <dassert.h>
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

GluiHandler::GluiHandler(const std::string& name) :
  GuiHandler(name),
  no_gui(true)
{
  //
}

GluiHandler::~GluiHandler()
{
  //
}

void GluiHandler::init(bool xlib_lock)
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
}

#ifdef HAVE_GLUTMAINLOOPEVENT
void GluiHandler::passControl()
{
  assert(hooks_done);
  cout << "Low prio thread starts cyclic running with GLUI" << endl;
  in_gui = true;

  while (in_gui) {
    CSE.doLoop();
    if (glutGetWindow()) {
      glutMainLoopEvent();
      glui_idle_func();
    }
  }
}

void GluiHandler::returnControl()
{
  in_gui = false;
}

#else
// using traditional GLUT

static void call_environment_loop()
{
  CSE.doLoop();
}

void GluiHandler::passControl()
{
  // we do two things. While there is no window (glutGetWindow returns
  // 0), we handle timing and updates from here. As soon as a window
  // has been created, go and use the glut loop
  DEB("Entering glut control");
  while (glutGetWindow() == 0 && no_gui) {
    CSE.doLoop();
  }

  if (glutGetWindow() == 0) {
    // still no window, and I was stopped before making it
    return;
  }
  no_gui = false;

  // At this point, there must be a window, so I can call glutMainLoop
  DEB("Setting timer func");
  GLUI_Master.set_glutIdleFunc( call_environment_loop );

  DEB("Entering main loop forever");
  glutMainLoop();
}

void GluiHandler::returnControl()
{
  if (no_gui) {
    // have not even started GLUT yet. Just set no_gui to false, and
    // the main loop will stop
    DEB("Nicely telling to stop");
    no_gui = false;
    return;
  }

  DEB("Glui calling exit");
  std::exit(0);
  // just hope this did not do anything terrible to the windowing code
}

#endif


DUECA_NS_END

