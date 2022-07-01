/* ------------------------------------------------------------------   */
/*      item            : DuecaGLWindow.cxx
        made by         : Rene' van Paassen
        date            : 010409
        category        : body file
        description     :
        changes         : 010409 first version
                          010427 STANDALONE possibility. If the
                          STANDALONE flag is set, a testing version is
                          generated. In combination with the main
                          routine in VisualMain, you can create a test
                          window, with the same code that can also run
                          in DUECA.
                          040701 Addition of FLTK interface
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define DuecaGLWindow_cxx

#include <dueca-conf.h>
#include "DuecaGLWindow.hxx"
#include "OpenGLHelper.hxx"
#include "Environment.hxx"

// The dueca version tries to read the config, and uses the environment
// to find out which graphic interface is used (and thus how to start
// up).

#include <debug.h>

DUECA_NS_START;

// constructor of the object
DuecaGLWindow::DuecaGLWindow(const char* window_title,
                             bool pass_passive) :
  DuecaGLCanvas(),
  window_title(window_title),
  mouse_passive(pass_passive),
  fullscreen(false),
  offset_x(-1),
  offset_y(-1),
  size_x(400),
  size_y(300)
{
  //
}

// backwards compatibility constructor
DuecaGLWindow::DuecaGLWindow(const char* window_title,
                             bool dummy1,
                             bool dummy2,
                             bool dummy3,
                             bool dummy4,
                             bool mouse_passive) :
  DuecaGLCanvas(),
  window_title(window_title),
  mouse_passive(mouse_passive),
  fullscreen(false),
  offset_x(-1),
  offset_y(-1),
  size_x(400),
  size_y(300)
{
  //
}

DuecaGLWindow::~DuecaGLWindow()
{
  //
}

bool DuecaGLWindow::setFullScreen(const bool& fs)
{
  fullscreen = fs;
  return true;
}

bool DuecaGLWindow::setWindow(const std::vector<int>& wpos)
{
  /* should be an assertion or even better an exception */
  if (wpos.size() != 4) return false;

  setWindow(wpos[0], wpos[1], wpos[2], wpos[3]);
  return true;
}

void DuecaGLWindow::setWindow(int posx, int posy,
                              int width, int height)
{
  fullscreen = false;
  offset_x = posx;
  offset_y = posy;
  size_x = width;
  size_y = height;
}

void DuecaGLWindow::openWindow(int priority)
{
  // get out if this window was already opened
  if (helper != NULL) return;

  // is there a choice corresponding to the windowing choice?

  // find out what the current gui is. Depending on this, select the
  // methods to open a gl window

  // if there is a GL windowing toolkit with sweeper in my thread,
  // prefer that one to the default (normally gtk or gtk2 toolkit)
  for (std::map<std::string, OpenGLHelper*>::iterator ii =
         OpenGLHelper::all().begin(); ii != OpenGLHelper::all().end(); ii++) {
    if (ii->second->haveSweeper()) {
      /* DUECA extra.

         Information on GL sweeper use. */
      I_XTR("GL Window uses sweeper of type " << ii->first);
      helper = ii->second->newWindow();
      helper->open(window_title, this, offset_x, offset_y, size_x, size_y,
                   fullscreen);
      return;
    }
  }

  // At this point, no sweeper found, use GL capability of
  // configured toolkit
  string choice = string(CSE.getGraphicInterface());
  if (OpenGLHelper::all().find(choice) != OpenGLHelper::all().end()) {
    helper = OpenGLHelper::all()[choice]->newWindow();
    helper->open(window_title, this, offset_x, offset_y, size_x, size_y,
                 fullscreen);
    return;
  }

  // Problem here, no GL capability
  /* DUECA extra.

     GL window is not possible, because GL drawing capability is
     lacking */
  E_XTR("DuecaGLWindow \"" << window_title <<
        "\" cannot find GL drawing capability!");
}

void DuecaGLWindow::hide()
{
  if (helper) helper->hide();
}

void DuecaGLWindow::show()
{
  if (helper) helper->show();
}

void DuecaGLWindow::windowDestroyed()
{
  helper = NULL;
}

void DuecaGLWindow::mouse(int button, int state, int x, int y)
{
  // nothing, apparently
}

void DuecaGLWindow::motion(int x, int y)
{
  // nothing, apparently
}

void DuecaGLWindow::passive(int x, int y)
{
  // nothing, apparently
}

void DuecaGLWindow::keyboard(unsigned char c, int x, int y)
{
  // nothing, apparently
}

void DuecaGLWindow::special(int key, int x, int y)
{
  // nothing, apparently
}

DUECA_NS_END;
