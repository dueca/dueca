/* ------------------------------------------------------------------   */
/*      item            : FltkHandler.cxx
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


#define FltkHandler_cxx

#include <dueca-conf.h>

#ifdef USE_FLTK

#ifdef ALSO_INIT_GLUT
#if defined(HAVE_GL_FREEGLUT_H)
#include <GL/freeglut_std.h>
#include <GL/freeglut_ext.h>
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#endif
#endif

#include "FltkHandler.hxx"
#include <iostream>
#include "Environment.hxx"
#include <dassert.h>

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

// not normal to put it so late, but re-defines None!@@@@
#ifdef HAVE_FL_FL_H
#include <FL/Fl.H>
#endif

DUECA_NS_START

FltkHandler::FltkHandler(const std::string& name) :
  GuiHandler(name)
{
  //
}

FltkHandler::~FltkHandler()
{

}

extern int* p_argc;
extern char*** p_argv;

void FltkHandler::init(bool xlib_lock)
{
#ifdef HAVE_X11_XLIB_H
  if (xlib_lock) {
    if (!XInitThreads()) {
      cerr << "Xlib thread initiation failed" << endl;
    }
  }
#endif
#ifdef ALSO_INIT_GLUT
  if (haveToInitialiseGlut()) {
    glutInit(p_argc, *p_argv);
  }
  runHooks();
#endif
}

void FltkHandler::passControl()
{
  assert(hooks_done);
  cout << "Low prio thread starts cyclic running with FLTK" << endl;
  in_gui = true;
  while (in_gui) {
    CSE.doLoop();
    Fl::check();
  }
}

void FltkHandler::returnControl()
{
  in_gui = false;
}

DUECA_NS_END

#endif
