/* ------------------------------------------------------------------   */
/*      item            : GuiHandler.cxx
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


#define GuiHandler_cxx
#include <dueca-conf.h>

#include "GuiHandler.hxx"
#include "Environment.hxx"
#include <iostream>
#include <dassert.h>
#include "StartIOStream.hxx"
#include "DuecaEnv.hxx"

DUECA_NS_START

bool GuiHandler::glut_initialised = false;
bool GuiHandler::hooks_done = false;

std::map<std::string, GuiHandler*> &GuiHandler::all()
{
  static std::map<std::string, GuiHandler*>_all;
  return _all;
}

std::list<initf> &GuiHandler::hooks()
{
  static std::list<initf> _hooks;
  return _hooks;
}

GuiHandler::GuiHandler(const std::string& name) :
  in_gui(false),
  guiname(name)
{
  // ensure iostream is available befor main.
  startIOStream();

  assert(all().find(name) == all().end());

  if (!DuecaEnv::scriptSpecific()) {
    cout << "Adding GUI    \"" << name << '"' << endl;
  }

  // add to list of potential gui
  all()[name] = this;
}

void GuiHandler::init(bool xlib_lock)
{
  runHooks();
}

GuiHandler::~GuiHandler()
{
  assert(in_gui == false);
}

void GuiHandler::passControl()
{
  assert(hooks_done);
  cout << "Low prio thread starts cyclic running" << endl;
  in_gui = true;
  while (in_gui) {
    CSE.doLoop();
  }
}

void GuiHandler::returnControl()
{
  in_gui = false;
}

void GuiHandler::addInitHook(initf hook)
{
  assert(!hooks_done);
  hooks().push_back(hook);
}

void GuiHandler::runHooks()
{
  assert(!hooks_done);
  for (list<initf>::iterator h = hooks().begin(); h != hooks().end(); h++) {
    (*h)(guiname);
  }
  hooks_done = true;
}

DUECA_NS_END
