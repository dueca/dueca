
#include <dueca/visibility.h>
#include <StartIOStream.hxx>
#include <iostream>
#include "GlutOpenGLHelper.hxx"
#include "DuecaEnv.hxx"
#include "TypeCreator.hxx"
#include <string>
#include <dueca_ns.h>

#define DO_INSTANTIATE
#include "TypeCreator.hxx"

USING_DUECA_NS;


extern "C"
LNK_PUBLICC void InitExtraGlut()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-extra-glut]" << std::endl;
  }
  static GlutOpenGLHelper h(std::string("glut"));
}


