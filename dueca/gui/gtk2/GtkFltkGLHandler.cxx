/* ------------------------------------------------------------------   */
/*      item            : GtkFltkGLHandler.cxx
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


#define GtkFltkGLHandler_cxx

#include <dueca-conf.h>
#include "GtkFltkGLHandler.hxx"
#include <iostream>
#ifdef HAVE_FL_FL_H
#include <FL/Fl.H>
#endif
#ifdef HAVE_GTK_GTK_H
#include <gtk/gtk.h>
#endif

#include "Environment.hxx"

#ifdef ALSO_INIT_GLUT
#if defined(HAVE_GL_FREEGLUT_H)
#include <GL/freeglut_std.h>
#include <GL/freeglut_ext.h>
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#endif
#endif

#if  defined(USE_GTK2) && defined(USE_FLTK)
DUECA_NS_START

GtkFltkGLHandler::GtkFltkGLHandler(const std::string& name) :
  GuiHandler(name)
{
  //
}

GtkFltkGLHandler::~GtkFltkGLHandler()
{
  //
}

extern "C" {
  static gint call_environment_loop(gpointer);
}

static gint call_environment_loop(gpointer )
{
  CSE.doLoop();
  Fl::check();
  return TRUE;
}

extern int* p_argc;
extern char*** p_argv;

void GtkFltkGLHandler::init()
{
  gtk_set_locale();
  gtk_init(p_argc, p_argv);
#ifdef ALSO_INIT_GLUT
  if (haveToInitialiseGlut()) {
    glutInit(p_argc, *p_argv);
  }
#endif
}


void GtkFltkGLHandler::passControl()
{
  cout << "Passing control to gtk+fltk-gl" << endl;

  gtk_idle_add(call_environment_loop, NULL);
  gtk_main();
}

void GtkFltkGLHandler::returnControl()
{
  in_gui = false;
  cerr << "Calling gtk_main_quit()" << endl;
  gtk_main_quit();
}

DUECA_NS_END

#endif
