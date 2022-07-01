/* ------------------------------------------------------------------   */
/*      item            : GlutProtocol.cxx
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
#include "GlutProtocol.hxx"
#include <Environment.hxx>
#include <GuiHandler.hxx>

#if defined(HAVE_GL_FREEGLUT_H)
#include <GL/freeglut.h>
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#elif defined(HAVE_GLUT_H)
#include <glut.h>
#endif

DUECA_NS_START

GlutProtocol::GlutProtocol() :
  WindowingProtocol("glut")
{
  //
}

bool GlutProtocol::complete()
{
  return true;
}

GlutProtocol::~GlutProtocol()
{
  //
}

bool GlutProtocol::init()
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

bool GlutProtocol::close()
{
  return true;
}

void GlutProtocol::sweep()
{
  if (glutGetWindow()) {
    glutMainLoopEvent();
  }
}

DUECA_NS_END

