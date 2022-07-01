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

#include <dueca/scriptinterface.h>
#include <dueca/Ticker.hxx>
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
#include <ModuleCreator.hxx>
#include <PythonScripting.hxx>
#include <dueca/Accessor.hxx>
#include <ScriptInterpret.hxx>
#include <dueca/GenericPacker.hxx>
#include <dueca/CreationCenter_Python.hxx>
#include <dueca/FillPacker.hxx>
#include <iostream>
#include "Unpacker.hxx"
#include "Packer.hxx"
#include "FillUnpacker.hxx"
#include "FillPacker.hxx"



#define DO_INSTANTIATE
#include "PythonStart.hxx"
#include "CoreCreator.hxx"
#include "TypeCreator.hxx"
#include "MemberCall.hxx"
#include "MemberCall2Way.hxx"
#include "Entity_Python.hxx"

// must be last includes
#include <dueca/debug.h>
#define DEBRPINTLEVEL 0
#include <debprint.h>


DUECA_NS_START

void init_dueca_accessor(void)
{
  static CoreCreator<Accessor,ScriptCreatable>
    ac("Accessor", NULL);
}

void init_dueca_genericpacker(void)
{
  static CoreCreator<GenericPacker,ScriptCreatable> g0
    ("GenericPacker", NULL);
}

// default name for void/no parent
template<>
const char* core_creator_name<mpl_::void_>(const char*)
{ return NULL; }

static void init_dueca_python(void)
{
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca]" << std::endl;
  }

  // set the proper scripting helper
  static PythonScripting scripter;

  // base class for most created objects, not created individually
  static CoreCreator<ScriptCreatable,mpl_::void_>
    sc("ScriptCreatable", NULL);

  static CoreCreator<Environment,ScriptCreatable>
    env(Environment::getParameterTable(),
        "Environment");

  // These are for transport. Can they be transferred to specific init files??
  static CoreCreator<PackerSet,ScriptCreatable,
                     GenericPacker&,
                     GenericPacker&,
                     GenericPacker&>
    ps(PackerSet::getParameterTable(),
       "PackerSet");
  static CoreCreator<PackerManager,ScriptCreatable>
    pm(PackerManager::getParameterTable(),
       "PackerManager");

  // the following two are the only ones still with the "GuileStart"
  // mode, since these do not fit the corecreator framework
  ScriptInterpret::addInitFunction("Entity", NULL, python_entity_init);
  ScriptInterpret::addInitFunction("Module", NULL, python_module_init);

  // the rest are all CoreCreator compatible, i.e. empty constructor
  // and a ParameterTable.
  static CoreCreator<Ticker,ScriptCreatable>
    tick(Ticker::getParameterTable(),
       "Ticker");
  static CoreCreator<PeriodicTimeSpec,ScriptCreatable,
                     bpy::optional<int,int> >
    spec(PeriodicTimeSpec::getParameterTable(), "TimeSpec");

  //static CoreCreator<PrioritySpec,ScriptCreatable,
  //                   DefParam<int,0,T_STR("priority")>,
  //                   DefParam<int,0,T_STR("order")> >
  //  prio(PrioritySpec::getParameterTable(), "PrioritySpec");
  static CoreCreator<PrioritySpec,ScriptCreatable,
                     int,int>
    prio(PrioritySpec::getParameterTable(), "PrioritySpec");
  static CoreCreator<ObjectManager,ScriptCreatable,int,int>
    om(ObjectManager::getParameterTable(), "ObjectManager");
  static CoreCreator<ChannelManager,ScriptCreatable>
    cm(ChannelManager::getParameterTable(), "ChannelManager");
  static GuiHandler h(std::string("none"));

  // the following creates a singleton. It will be replaced by a
  // derived object if in the configuration file a viewer is created.
  new DuecaView();
}

void init_dueca_ipdeps()
{
#if SCRIPT_PYTHON
  init_dueca_genericpacker();
#endif
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

static SetScriptInitFunction py(init_dueca_python);

// specific implementations from CoreCreatorPython.ixx
template<>
ProbeType getProbeType(const typeflag<NOOP>& a)
{
  return Probe_Sentinel;
}

template<>
ProbeType getProbeType(const typeflag<dueca::GenericPacker&>& a)
{
  return Probe_GenericPacker;
}


DUECA_NS_END
