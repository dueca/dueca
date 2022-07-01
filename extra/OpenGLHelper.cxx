/* ------------------------------------------------------------------   */
/*      item            : OpenGLHelper.cxx
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


#define OpenGLHelper_cxx
#include "OpenGLHelper.hxx"
#include "DuecaEnv.hxx"
#include <iostream>
#include <dassert.h>

DUECA_NS_START;

GLWindowHelper::GLWindowHelper()
{
  //
}

GLWindowHelper::~GLWindowHelper()
{
  //
}

void GLWindowHelper::open(const std::string &title, DuecaGLWindow* master,
          int offset_x, int offset_y, int width, int height,
          bool fullscreen)
{
  // nothing here
}

std::map<std::string, OpenGLHelper*> &OpenGLHelper::all()
{
  static std::map<std::string, OpenGLHelper*>_all;
  return _all;
}

OpenGLHelper::OpenGLHelper(const std::string &name) :
  priority(-1)
{
  assert(all().find(name) == all().end());

  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Adding GL for \"" << name << "\"" << std::endl;
  }

  // add to list of potential gui
  all()[name] = this;
}

OpenGLHelper::~OpenGLHelper()
{
  //
}

bool OpenGLHelper::setSweeper(int priority)
{
  if (this->priority != -1 || priority < 0) return false;
  this->priority = priority;
  return true;
}


DUECA_NS_END;
