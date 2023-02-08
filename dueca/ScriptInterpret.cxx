/* ------------------------------------------------------------------   */
/*      item            : ScriptInterpret.cxx
        made by         : Rene' van Paassen
        date            : 990701
        category        : body file
        description     :
        changes         : 990701 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ScriptInterpret_cc

#include <dueca-conf.h>
#include "ScriptInterpret.hxx"
#define I_CNF
#define E_CNF
#include <debug.h>
#include <dueca/SimTime.hxx>
#include <dueca/ScriptLine.hxx>
#include <dueca/ObjectManager.hxx>
#include <dueca/NodeManager.hxx>
#include <dueca/ScriptConfirm.hxx>
#include <dueca/DataReader.hxx>
#include <dueca/ScriptHelper.hxx>
#include <dueca/ChannelReadToken.hxx>
#include <dueca/ChannelWriteToken.hxx>
#include <dueca/WrapSendEvent.hxx>
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <stringoptions.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#define DEBPRINTLEVEL -1
#include <debprint.h>

#define DO_INSTANTIATE
#include <dueca/Callback.hxx>

DUECA_NS_START

InitFunction::InitFunction(const char* name, const char* parent, voidfunc func) :
  name(name), parent(parent), func(func)
{ }

void InitFunction::operator () (void) const
{
  (*func)();
}

ScriptInterpret* ScriptInterpret::singleton = NULL;

ScriptInterpret::ScriptInterpret() :
  helper(NULL),
  init_functions(),
  scriptinit(NULL),
  in_script(false),
  model_copied(false),
  received_sets(0),
  sent_sets(1),
  token_valid(this, &ScriptInterpret::tokenValid),
  token_action(true),
  w_creation(NULL),
  t_creation(NULL),
  w_confirm(NULL),
  t_confirm(NULL),
  w_goahead(NULL),
  cb1(this, &ScriptInterpret::handleConfigurationLines),
  cb2(this, &ScriptInterpret::checkConfirms),
  handle_lines(NULL),
  process_confirm(NULL)
{
  if (singleton != NULL) {
    cerr << "creation of a second Script interpreter attempted!" << endl;
    throw(scriptexception());
  }
}

void ScriptInterpret::completeCreation()
{
  handle_lines = new ActivityCallback(getId(), "script config line read", &cb1,
                                      PrioritySpec(0,0));
  process_confirm = new ActivityCallback(getId(), "confirm receive", &cb2,
                                         PrioritySpec(0,0));

  // initiate the communication with other scheme interpreters
  if (ObjectManager::single()->getLocation() == 0) {
    w_creation = new ChannelWriteToken
      (getId(), NameSet("dueca", "ScriptLine", ""),
       getclassname<ScriptLine>(), "script lines",
       Channel::Events, Channel::OnlyOneEntry,
       Channel::OnlyFullPacking, Channel::Bulk, &token_valid);

    t_confirm = new ChannelReadToken
      (getId(), NameSet("dueca", "ScriptConfirm", ""),
       getclassname<ScriptConfirm>(), entry_any,
       Channel::Events, Channel::OneOrMoreEntries,
       Channel::AdaptEventStream, 0.0, &token_valid);

    w_goahead = new ChannelWriteToken
      (getId(), NameSet("dueca", "ScriptConfirm", "processadditional"),
       getclassname<ScriptConfirm>(), "script go ahead",
       Channel::Events, Channel::OnlyOneEntry,
       Channel::OnlyFullPacking, Channel::Regular, &token_valid);

    process_confirm->setTrigger(*t_confirm);
    process_confirm->switchOn(TimeSpec(0,0));
  }
  t_creation = new ChannelReadToken
      (getId(), NameSet("dueca", "ScriptLine", ""),
       getclassname<ScriptLine>(), 0,
       Channel::Events, Channel::OnlyOneEntry,
       Channel::AdaptEventStream, 0.0, &token_valid);

  w_confirm = new ChannelWriteToken
    (getId(), NameSet("dueca", "ScriptConfirm", ""),
     getclassname<ScriptConfirm>(), std::string("script confirm ") +
     boost::lexical_cast<std::string>
     (unsigned(ObjectManager::single()->getLocation())),
     Channel::Events, Channel::OneOrMoreEntries,
     Channel::OnlyFullPacking, Channel::Regular,  &token_valid);

  handle_lines->setTrigger(*t_creation);
  handle_lines->switchOn(TimeSpec(0,0));

  confirmation_count.resize(NodeManager::single()->getNoOfNodes(), 0);
}

void ScriptInterpret::tokenValid(const TimeSpec& ts)
{
  if (w_creation != NULL) w_creation->isValid();
  if (t_creation != NULL) t_creation->isValid();
  if (w_confirm != NULL) w_confirm->isValid();
  if (t_confirm != NULL) t_confirm->isValid();
  if (w_goahead != NULL) w_goahead->isValid();
}

ScriptInterpret::~ScriptInterpret()
{
  delete helper;
  if (this == singleton) singleton = NULL;
}

ScriptInterpret* ScriptInterpret::single(ScriptHelper* helper)
{
  if (ScriptInterpret::singleton == NULL) {
    ScriptInterpret::singleton = new ScriptInterpret();
  }
  if (helper) {
    if (singleton->helper) {
      cerr << "attempt to repeatedly define script language" << endl;
      throw (scriptexception());
    }
    singleton->helper = helper;
    helper->initiate();
  }
  if (!singleton->helper) {
    cerr << "no script language defined" << endl;
      throw (scriptexception());
  }
  return singleton;
}

void ScriptInterpret::startScript()
{
  singleton->sortInitFunctions();
  singleton->in_script = true;
  helper->interpreter();
  // they say this never returns
}

void ScriptInterpret::initializeScriptLang()
{
  if (singleton->scriptinit) {
    (*(singleton->scriptinit))();
  }
  else {
    cerr << "No script language defined" << endl;
  }
}

void ScriptInterpret::createObjects()
{
  assert(in_script);

  if (ObjectManager::single()->getLocation() == 0) {
    std::string buff;

    // get a time for the data
    TimeTickType send_time = SimTime::now();

    // copy the basic configuration file to the pipe
    while (helper->readline(buff)) {
      wrapSendData(*w_creation, new ScriptLine(buff), send_time);
    }

    // copy closing off
    wrapSendData
      (*w_creation, new ScriptLine(helper->phase2), send_time);
    wrapSendData
      (*w_creation, new ScriptLine(helper->stopsign), send_time);

    /// flag that we have to wait for the model to be copied over
    model_copied = false;
  }
}

bool ScriptInterpret::readAdditionalModuleConf(const vstring& fname)
{
  if (ObjectManager::single()->getLocation() == 0) {
    // read module changes from file. Transmit them over the module
    // creation channel
    ifstream mod(fname.c_str());
    if (!mod.good()) {
      cerr << "Error opening " << fname << endl;
      return false;
    }
    TimeTickType send_time = SimTime::now();

    vstring buff;
    // copy the basic configuration file to the pipe
    while (getline(mod, buff)) {
      wrapSendEvent(*w_creation, new ScriptLine(buff), send_time);
    }

    // copy closing off
    wrapSendEvent
      (*w_creation, new ScriptLine(helper->phase3), send_time);
    wrapSendEvent
      (*w_creation, new ScriptLine(helper->stopsign), send_time);

    // flag that we have to wait for the model to be copied over
    model_copied = false;

    /* DUECA scripting.

       Information on reading additional script code from a given
       file.
    */
    I_SYS("Read additional model data from \"" << fname << "\"");
  }
  return true;
}

void ScriptInterpret::writeQuit()
{
  if (ObjectManager::single()->getLocation() == 0) {

    TimeTickType send_time = SimTime::now();

    // prevent that we are thinking this is normal additional
    // configuration
    sent_sets = 0xffff;

    // copy closing off
    wrapSendEvent
      (*w_creation, new ScriptLine(helper->quitline), send_time);
    wrapSendEvent
      (*w_creation, new ScriptLine(helper->stopsign), send_time);

    model_copied = false;
  }
}

void ScriptInterpret::handleConfigurationLines(const TimeSpec& time)
{
  if (t_creation->haveVisibleSets(time)) {
    DataReader<ScriptLine, VirtualJoin> e(*t_creation);

    if (helper->writeline(e.data().line)) {

      // send a confirmation that a model set has been received
      wrapSendEvent(*w_confirm, new ScriptConfirm(++received_sets),
                    time.getValidityStart());

      // flag that the model has been copied, but not for node 0,
      // because 0 will have to wait for the confirmation from all others
      if (NodeManager::single()->getThisNodeNo() != 0) {
        model_copied = true;
      }
    /* DUECA scripting.

       At this point the model code has been copied to the other DUECA
       nodes.
    */
      I_SYS("Model completely copied, confirm no=" << received_sets);
    }
    else {

      // we have got data, but it is not complete
      model_copied = false;
    }
  }
}

void ScriptInterpret::checkConfirms(const TimeSpec& time)
{
  DataReader<ScriptConfirm, VirtualJoin> conf(*t_confirm, time);

  confirmation_count[conf.origin().getLocationId()] =
    conf.data().confirm_no;

  bool check_copy = true;
  for (int ii = confirmation_count.size(); check_copy && ii--; ) {
    check_copy = (confirmation_count[ii] == received_sets);
  }

  if (check_copy && received_sets > sent_sets) {
    sent_sets = received_sets;
    DataWriter<ScriptConfirm> go(*w_goahead, time);
    go.data().confirm_no = sent_sets;
  }

  model_copied = check_copy;
}

void ScriptInterpret::addInitFunction(voidfunc ifunct)
{
  addInitFunction("anonymous", NULL, ifunct);
}


void ScriptInterpret::addInitFunction(const char* name,
                                      const char* parent, voidfunc ifunct)
{
  addInitFunction(new InitFunction(name, parent, ifunct));
}

void ScriptInterpret::addInitFunction(const InitFunction *ifunct)
{
  if (!singleton) singleton = new ScriptInterpret();

  // no parent, then push in front
  if (ifunct->parent == NULL) {
    DEB("Start function, no parent, inserting init for " << ifunct->name);
    singleton->init_functions.push_front(ifunct);
  }
  else {
    singleton->unsorted_functions.push_back(ifunct);
  }
}


void ScriptInterpret::sortInitFunctions()
{
  for (auto i0 = init_functions.begin();
       i0 != init_functions.end(); i0++) {

    // copy, to leave free for inserting, etc.
    auto ip = i0;
    DEB("Start function, are there children for " << (*ip)->name);

    // check the potential children for each unparented entry
    for (auto it = unsorted_functions.begin();
         it != unsorted_functions.end(); ) {

      DEB("Start function, checking " << (*it)->name << " parent " << (*it)->parent);

      // check if this is a child
      if (!strcmp((*i0)->name, (*it)->parent)) {

        DEB("Start function, found a child " << (*it)->name <<
            " for " << (*i0)->name);

        // insert this *after* the parent, or any previously inserted child
        ip = init_functions.insert(++ip, *it);

        // and remove from the unsorted
        it = unsorted_functions.erase(it);
      }
      else {
        // pass over
        it++;
      }
    }
  }

  for (auto it = unsorted_functions.begin();
       it != unsorted_functions.end(); it++) {
    /* DUECA scripting.

       A class that is available to the scripting language is missing its
       parent class, and it cannot be inserted at the right position.
     */
    W_SYS("Script init, cannot find parent " << (*it)->parent <<
          " for " << (*it)->name);
  }
}

const InitFunction* ScriptInterpret::getNextInitFunction()
{
  const InitFunction *r = NULL;
  if (init_functions.size()) {
    r = init_functions.front();
    init_functions.pop_front();
  }
  return r;
}

AddInitFunction::AddInitFunction(const char* name, voidfunc ifunct,
                                 const char* parent)
{
  ScriptInterpret::addInitFunction(name, parent, ifunct);
}

void ScriptInterpret::runCode(const char* code)
{
  helper->runCode(code);
}

SetScriptInitFunction::SetScriptInitFunction(voidfunc ifunct)
{
  if (!ScriptInterpret::singleton)
    ScriptInterpret::singleton = new ScriptInterpret();
  if (ScriptInterpret::singleton->scriptinit) {
    std::cerr << "Attempt to set a second script language" << std::endl;
    throw(scriptexception());
  }
  ScriptInterpret::singleton->scriptinit = ifunct;
}

DUECA_NS_END
