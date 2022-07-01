/* ------------------------------------------------------------------   */
/*      item            : Init.cxx
        made by         : Rene' van Paassen
        date            : ???
        category        : Body, init file
        description     :
        changes         : ??????? first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

//#define OLD_SCRIPT_INTERFACE

#include "scriptinterface.h"
#include "dueca_assert.h"
#include "Init.hxx"
#include "Ticker.hxx"
#include "Environment.hxx"
#include "PackerSet.hxx"
#include "PackerManager.hxx"
#include "TimeSpec.hxx"
#include "DuecaEnv.hxx"
#include "PrioritySpec.hxx"
#include "ObjectManager.hxx"
#include "WindowingProtocol.hxx"
#include "ChannelManager.hxx"
#include "GuiHandler.hxx"
#include "DuecaView.hxx"
#include <iostream>
#include <SchemeScripting.hxx>
#include <SchemeClassData.hxx>
#include <debug.h>
#include "Unpacker.hxx"
#include "Packer.hxx"
#include "FillUnpacker.hxx"
#include "FillPacker.hxx"

#define DO_INSTANTIATE
#include "GuileStart.hxx"
#include "CoreCreator.hxx"
#include "TypeCreator.hxx"
#include "MemberCall.hxx"
#include "MemberCall2Way.hxx"


DUECA_NS_START

// ScriptCreatable has no parent, and is therefore special in instantiation
template<>
SchemeClassData<ScriptCreatable> * SchemeClassData<ScriptCreatable>::single()
{
  static SchemeClassData<ScriptCreatable> singleton
    ("script-creatable", NULL);
  return &singleton;
}

SCHEME_CLASS_SINGLE(PrioritySpec,ScriptCreatable,"priority-spec");

DUECA_NS_END

#include "Entity_Scheme.hxx"
#include "CreationCenter_Scheme.hxx"

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

bool reflective_components_init_done = false;
bool copying_components_init_done = false;

static SCM main_thread(SCM stage)
{
  DUECA_SCM_ASSERT(scm_is_exact_integer(stage) && scm_to_int(stage) >= 0,
                   stage, SCM_ARG1, "pass-control");

  Environment::getInstance()->proceed(scm_to_int(stage));
  return SCM_BOOL_T;
}

static SCM set_graphic_interface(SCM iface)
{
  // check that the first argument is a literal with a valid gui
  // library name
  /* DUECA scripting.

     A warning on the obsolete set-graphic-interface call. Fix your
     dueca.cnf to resolve this problem. */
  W_CNF("The set-graphic-interface call is deprecated, " << endl <<
        "use 'graphic-interface in the make-environment call instead");

  /*  DUECA_SCM_ASSERT(SCM_NIMP(iface) && SCM_SYMBOLP(iface) &&
                   checkGuiName(string(dueca_scm_chars(iface))),
                   iface, SCM_ARG1, "set-graphic-interface"); */
  Environment::getInstance()->setGraphicInterface(dueca_scm_chars(iface));

  return SCM_BOOL_T;
}

extern SCM main_thread(SCM stage);
extern SCM set_graphic_interface(SCM iface);

// template specializations for scheme creatable classes class
// information data

SCHEME_CLASS_SINGLE(GenericPacker,ScriptCreatable,"generic-packer");
SCHEME_CLASS_SINGLE(Unpacker,ScriptCreatable,"unpacker");
SCHEME_CLASS_SINGLE(Packer,GenericPacker,"packer");
SCHEME_CLASS_SINGLE(FillUnpacker,ScriptCreatable,"fill-unpacker");
SCHEME_CLASS_SINGLE(FillPacker,GenericPacker,"fill-packer");
SCHEME_CLASS_SINGLE(Environment,ScriptCreatable,"environment");
SCHEME_CLASS_SINGLE(PackerSet,ScriptCreatable,"packer-set");
SCHEME_CLASS_SINGLE(PackerManager,ScriptCreatable,"packer-manager");
SCHEME_CLASS_SINGLE(Ticker,ScriptCreatable,"ticker");
SCHEME_CLASS_SINGLE(PeriodicTimeSpec,ScriptCreatable,"time-spec");
SCHEME_CLASS_SINGLE(ObjectManager,ScriptCreatable,"object-manager");
SCHEME_CLASS_SINGLE(ChannelManager,ScriptCreatable,"channel-manager");
SCHEME_CLASS_SINGLE(Accessor,ScriptCreatable,"accessor");
SCHEME_CLASS_SINGLE(WindowingProtocol,ScriptCreatable,"windowing-protocol");

static void add_environment_calls()
{
  scm_c_define_gsubr("pass-control", 1, 0, 0, (scm_func) main_thread);
  scm_c_define_gsubr("set-graphic-interface", 1, 0, 0,
                     (scm_func) set_graphic_interface);
}

void init_dueca_accessor(void)
{
}

void init_dueca_genericpacker(void)
{
}

void init_dueca_ipdeps()
{
  static CoreCreator<Unpacker,ScriptCreatable>
    g1(Unpacker::getParameterTable(),
       ArgListProcessor::AllowListAndPair, NULL, "Unpacker");
  static CoreCreator<Packer,GenericPacker>
    g2(Packer::getParameterTable(),
       ArgListProcessor::AllowListAndPair, NULL, "Packer");
  static CoreCreator<FillUnpacker,ScriptCreatable>
    g3(FillUnpacker::getParameterTable(),
       ArgListProcessor::AllowListAndPair, NULL, "FillUnpacker");
  static CoreCreator<FillPacker,GenericPacker>
    g4 (FillPacker::getParameterTable(),
       ArgListProcessor::AllowListAndPair, NULL, "FillPacker");
}

static void init_dueca_scheme(void)
{
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca]" << std::endl;
  }
  static SchemeScripting scripter;
  new CoreCreator<Environment>(Environment::getParameterTable(),
                               ArgListProcessor::DeprecateList,
                               add_environment_calls,
                               "environment");

  // These are for transport. Can they be transferred to specific init files??
  new CoreCreator<PackerSet>
    (PackerSet::getParameterTable(), ArgListProcessor::AllowListAndPair,
     NULL, "packer-set");
  new CoreCreator<PackerManager>
    (PackerManager::getParameterTable(), "packer-manager");

  // the following two are the only ones still with the "GuileStart"
  // mode, since these do not fit the corecreator framework
  new GuileStart<Entity>();
  new GuileStart<CreationCenter>();

  // the rest are all CoreCreator compatible, i.e. empty constructor
  // and a ParameterTable.
  new CoreCreator<Ticker>
    (Ticker::getParameterTable(), ArgListProcessor::DeprecateList,
     NULL, "ticker");
  new CoreCreator<PeriodicTimeSpec>
    (PeriodicTimeSpec::getParameterTable(), ArgListProcessor::AllowListAndPair,
     NULL, "time-spec");
  new CoreCreator<PrioritySpec>
    (PrioritySpec::getParameterTable(), ArgListProcessor::AllowListAndPair,
     NULL, "priority-spec");
  new CoreCreator<ObjectManager>
    (ObjectManager::getParameterTable(), ArgListProcessor::DeprecateList,
     NULL, "object-manager");
  new CoreCreator<ChannelManager>
    (ChannelManager::getParameterTable(), ArgListProcessor::DeprecateList,
     NULL, "channel-manager");
  static GuiHandler h(std::string("none"));

  // the following creates a singleton. It will be replaced by a
  // derived object if in the configuration file a viewer is created.
  new DuecaView();
}

static SetScriptInitFunction sc(init_dueca_scheme);

DUECA_NS_END
