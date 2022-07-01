/* ------------------------------------------------------------------   */
/*      item            : GtkHandler.cxx
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


#define GtkHandler_cxx
#include <dueca-conf.h>

#include "GtkHandler.hxx"
#include <iostream>
#include <gtk/gtk.h>

#include "Environment.hxx"

#ifdef ALSO_INIT_GLUT
#if defined(HAVE_GL_FREEGLUT_H)
#include <GL/freeglut_std.h>
#include <GL/freeglut_ext.h>
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#else
#error "No glut headers, cannot honour ALSO_INIT_GLUT"
#endif
#endif

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif
#include <dassert.h>

#define I_SYS
#include <debug.h>

DUECA_NS_START

#if GTK_MAJOR_VERSION != 2
#error "Wrong GTK version headers!"
#endif

GtkHandler::GtkHandler(const std::string& name) :
  GuiHandler(name)
{
  //
}

GtkHandler::~GtkHandler()
{
  //
}

extern "C" {
  static gint call_environment_loop(gpointer);
}

extern int* p_argc;
extern char*** p_argv;

static gint call_environment_loop(gpointer )
{

#ifdef ALSO_INIT_GLUT
  // glut initialisation, for cases where glut convenience drawing
  // functions are used
  if (GuiHandler::haveToInitialiseGlut()) {
    glutInit(p_argc, *p_argv);
  }
#endif

  CSE.doLoop();
  return TRUE;
}


void GtkHandler::init(bool xlib_lock)
{
#ifdef HAVE_X11_XLIB_H
  if (xlib_lock) {
    if (!XInitThreads()) {
      cerr << "Xlib thread initiation failed" << endl;
    }
  }
#else
  if (xlib_lock) {
    cerr << "Cannot initialise Xlib lock, no headers" << endl;
  }
#endif
  /* DUECA graphics.

     Information on initialising graphics. */
  I_SYS("Initialising gtk2 graphics");
  gtk_init(p_argc, p_argv);

  runHooks();
}

void GtkHandler::passControl()
{
  assert(hooks_done);
  /* DUECA graphics.

     Information on passing control to graphics code. */
  I_SYS("Passing control to gtk 2.x");

  g_idle_add(call_environment_loop, NULL);
  gtk_main();
}

void GtkHandler::returnControl()
{
  in_gui = false;
  /* DUECA graphics.

     Information on commanding end of graphics code run. */
  I_SYS("Calling gtk_main_quit()");
  gtk_main_quit();
}
DUECA_NS_END

