/* ------------------------------------------------------------------   */
/*      item            : GluiProtocol.cxx
        made by         : Rene' van Paassen
        date            : 071112
        category        : body file
        description     :
        changes         : 071112 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include <dueca-conf.h>
#include "GluiProtocol.hxx"
#include <Environment.hxx>
#include <GuiHandler.hxx>
#include <dueca/ParameterTable.hxx>
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
#error "No glui.h headers"
#endif

DUECA_NS_START

const ParameterTable* GluiProtocol::getParameterTable()
{
  static ParameterTable table[] = {
    { NULL, NULL,
      "Specify interaction with the GLUI widget set" }
  };
  return table;
}

GluiProtocol::GluiProtocol() :
  WindowingProtocol("glui")
{
  //
}

bool GluiProtocol::complete()
{
  return true;
}

GluiProtocol::~GluiProtocol()
{
  //
}

bool GluiProtocol::init()
{
  // Initialize glut, but only if another windowing toolkit did not do
  // this first
  if (CSE.getActiveGuiHandler() &&
      CSE.getActiveGuiHandler()->haveToInitialiseGlut()) {
    static int argc = 1;
    static char* argv[] = {const_cast<char*>("dueca_run.x")};
    glutInit(&argc, argv);
  }

  return true;
}

bool GluiProtocol::close()
{
  return true;
}

void GluiProtocol::sweep()
{
  if (glutGetWindow()) {
    glutMainLoopEvent();
    glui_idle_func();
  }
}

DUECA_NS_END

