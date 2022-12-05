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

// modify, to create the app, and use app iteration

#define GtkHandler_cxx
#include <dueca-conf.h>

#include "GtkHandler.hxx"
#include <iostream>
#include <gtk/gtk.h>
#include <dassert.h>
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

#define DEBPRINTLEVEL 1
#include <debprint.h>

DUECA_NS_START

#if GTK_MAJOR_VERSION != 4
#error "Wrong GTK version headers!"
#endif

#define APPLICATION_ID "nl.tudelft.dueca.mainmenu"

GtkHandler::GtkHandler(const std::string& name) :
  GuiHandler(name),
  app(NULL)
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

extern int* p_argc;
extern char*** p_argv;

void app_activate(GApplication *application, gpointer user_data)
{
  DEB("APP activate");
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
    cerr << "Cannot initialise Xlib lock" << endl;
  }
#endif

  /* DUECA graphics.

     Information on initialising graphics. */
  I_SYS("Initializing gtk4 application");

  // create an application
  app = gtk_application_new(APPLICATION_ID, G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(app_activate),
  		   reinterpret_cast<gpointer>(this));
  g_application_hold(G_APPLICATION(app));
  
  runHooks();
}

void GtkHandler::passControl()
{
  assert(hooks_done);
  /* DUECA graphics.

     Information on passing control to graphics code. */
  I_SYS("Passing control to gtk 3.x");

  // set the idle callback
  g_idle_add(call_environment_loop, NULL);

  // transfer to gtk application
  int result = g_application_run(G_APPLICATION(app), *p_argc, *p_argv);
  g_object_unref(app);
  
  if (result != 0) {
    /* DUECA graphics.

       Result after application run is non-zero */
    W_MOD("Non-zero result from gtk application " << result);
  }
}

void GtkHandler::returnControl()
{
  in_gui = false;
  /* DUECA graphics.

     Information on commanding end of graphics code run. */
  I_SYS("Calling gtk_main_quit()");
  g_application_release(G_APPLICATION(app));
  //gtk_main_quit();
}
DUECA_NS_END


