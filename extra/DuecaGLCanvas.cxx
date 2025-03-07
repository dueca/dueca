/* ------------------------------------------------------------------   */
/*      item            : DuecaGLCanvas.cxx
        made by         : Joost Ellerbroek
        date            : 100625
        category        : body file
        description     :
        changes         : 100625 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/Rene van Paassen
        license         : EUPL-1.2
*/

#include <dueca-conf.h>

#include "DuecaGLCanvas.hxx"
#include "OpenGLHelper.hxx"

// The dueca version tries to read the config, and uses the environment
// to find out which graphic interface is used (and thus how to start
// up).

#define E_CNF
#define W_CNF
#define I_CNF
#include <debug.h>

DUECA_NS_START;

// constructor of the object
DuecaGLCanvas::DuecaGLCanvas() :
    helper(NULL),
    marked_for_redraw(false),
    gl_initialised(false)
{
  //
}

DuecaGLCanvas::~DuecaGLCanvas()
{
  delete helper;
}

void DuecaGLCanvas::makeCurrent()
{
  if (helper)  helper->current();
}

void DuecaGLCanvas::selectCursor(int i)
{
  if (helper)  helper->cursor(i);
}

void DuecaGLCanvas::redraw()
{
  if (helper)  helper->redraw();
}

void DuecaGLCanvas::swapBuffers()
{
  if (helper) helper->swap();
}

void DuecaGLCanvas::initGL()
{
  // nothing, apparently
}

void DuecaGLCanvas::reshape(int x, int y)
{
  // nothing, apparently
}

int DuecaGLCanvas::getWidth()
{
  return helper ? helper->width() : 0;
}

int DuecaGLCanvas::getHeight()
{
  return helper ? helper->height() : 0;
}

int DuecaGLCanvas::getXOffset()
{
  return helper ? helper->xoffset() : 0;
}

int DuecaGLCanvas::getYOffset()
{
  return helper ? helper->yoffset() : 0;
}

DUECA_NS_END;
