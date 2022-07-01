/* ------------------------------------------------------------------   */
/*      item            : CreationCenter_Scheme.hxx
        made by         : Rene van Paassen
        date            : 180219
        category        : header file
        description     :
        changes         : 180219 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CreationCenter_Scheme_hxx
#define CreationCenter_Scheme_hxx

#include <CreationCenter.hxx>
#include <ModuleCreator.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

template<>
SchemeClassData<CreationCenter>* SchemeClassData<CreationCenter>::single()
{
  static SchemeClassData<CreationCenter> singleton
    ("module", SchemeClassData<ScriptCreatable>::single());
  return &singleton;
}

// defines "module" to the script
#if defined(SCM_USE_FOREIGN)
static SCM scm_module_type;
#else
static unsigned int module_smob_id;
#endif

bool CreationCenter_validTag(SCM i)
{
#if defined(SCM_USE_FOREIGN)
  return SCM_IS_A_P(i, scm_module_type);
#else
  return SCM_SMOB_PREDICATE(module_smob_id, i);
#endif
}

#if defined(SCM_USE_FOREIGN)
static void finalize_module(SCM smob)
{
  DEB("Finalizing module");
  SchemeObject* obj = reinterpret_cast<SchemeObject*>
    (scm_foreign_object_ref(smob, size_t(0)));

  // explicitly call destructor, scheme will clean up memory
  obj->~SchemeObject();
}
#else
static SCM mark_object (SCM smob)
{
  //cerr << "module mark" << endl;
  reinterpret_cast<SchemeObject*>(SCM_SMOB_DATA(smob))->markReferred();
  return SCM_BOOL_F;
}

static scm_sizet free_object (SCM smob)
{
  /* dangerous, but try a delete */
  delete(reinterpret_cast<SchemeObject*>(SCM_SMOB_DATA(smob)));
  return 0;
}

static int print_object (SCM smob, SCM port, scm_print_state *pstate)
{
  scm_puts("#<module ", port);
  std::ostringstream p;
  SchemeObject* obj = reinterpret_cast<SchemeObject*>(SCM_SMOB_DATA(smob));
  ModuleCreator* mc = obj->getModuleCreator();

  p << mc->getEntityName()
    << ' '
    << mc->getType()
    << '>';

  scm_puts(const_cast<char*>(p.str().c_str()), port);
  return 1;
}
#endif

//static scm_smobfuns obj_funs = {
//  mark_object, free_object, print_object, 0
//};

// function for creating a module
static const char* s_make_module = "make-module";

SCM make_module(SCM module_type, SCM partname, SCM specifications)
{
  DEB("make_module " << dueca_scm_chars(module_type));

  DUECA_SCM_ASSERT(SCM_SYMBOLP(module_type) &&
                   DUECA_NS::CreationCenter::single()->
                   hasType(dueca_scm_chars(module_type)),
                   module_type, SCM_ARG1, s_make_module);
  DUECA_SCM_ASSERT(scm_is_string(partname),
                   partname, SCM_ARG2, s_make_module);

  // module type and part name are the first two arguments
  std::string mtype = dueca_scm_chars(module_type);
  std::string pname = dueca_scm_chars(partname);


  // there may be an optional priority-spec after the part name, as
  // first in the list with specifications
  ModuleCreator* mcptr = NULL;
  if (SCM_CONSP(specifications) &&
      SchemeClassData<PrioritySpec>::single()->
      validTag(SCM_CAR(specifications))) {
#if defined(SCM_USE_FOREIGN)
    SchemeObject* pobj = reinterpret_cast<SchemeObject*>
      (scm_foreign_object_ref(SCM_CAR(specifications), size_t(0)));
#else
    SchemeObject* pobj = reinterpret_cast<SchemeObject*>
      (SCM_SMOB_DATA(SCM_CAR(specifications)));
#endif

    mcptr = CreationCenter::single()->createModuleCreator
      (mtype, pname, *dynamic_cast<PrioritySpec*>(pobj->getObject()));

    specifications = SCM_CDR(specifications);
  }
  else {
    static PrioritySpec pdeflt(0, 0);
     mcptr = CreationCenter::single()->createModuleCreator
       (mtype, pname, pdeflt);
  }

  // link it to a new scheme object
#if defined(SCM_USE_FOREIGN)
  SchemeObject *sobj =
    new (scm_gc_malloc(sizeof(SchemeObject), mtype.c_str()))
    SchemeObject(mcptr);
  SCM fob = scm_make_foreign_object_1(scm_module_type, sobj);
#else
  SchemeObject* sobj = new SchemeObject(mcptr);
  SCM fob = dueca_new_smob(module_smob_id, sobj);
#endif

  // remember the current scheme object
  sobj->setSCM(fob);

  // add any parameters
  const ScriptTypeCreator* father = dynamic_cast<const ScriptTypeCreator*>
    (mcptr->getFather());
  assert(father != NULL);
  if (!father->processList(specifications, mcptr->processed(), *sobj)) {
    mcptr->argumentError();
  }

  // return resulting object
  return fob;
}

// function for modifying a module
static const char* s_modify_module = "modify-module";
SCM modify_module(SCM module, SCM specifications)
{
  DUECA_SCM_ASSERT(SCM_NIMP(module) && CreationCenter_validTag(module),
                   module, SCM_ARG1, s_modify_module);

#if defined(SCM_USE_FOREIGN)
  SchemeObject* sobj = reinterpret_cast<SchemeObject*>
    (scm_foreign_object_ref(module, 0));
#else
  // specifications may be checked by the ModuleCreator
  SchemeObject* sobj = reinterpret_cast<SchemeObject*>(SCM_SMOB_DATA(module));
#endif

  ModuleCreator* mc = sobj->getModuleCreator();

  // everything ok. supply specifications to the ModuleCreator
  // the addspecifications call will also call the complete method again.
  const ScriptTypeCreator* father = dynamic_cast<const ScriptTypeCreator*>
    (mc->getFather());
  assert(father != NULL);
  if (father->processList(specifications, mc->processed(), *sobj)) {
    mc->injectAndCheckComplete();
    DEB("modified module " << mc->getName());
  }
  else {
    mc->argumentError();
  }

  // return nothing
  return SCM_UNSPECIFIED;
}

template<>
void scheme_init<CreationCenter>()
{
#if defined(SCM_USE_FOREIGN)
  SCM name = scm_from_utf8_symbol("module");
  SCM slots = scm_list_1(scm_from_utf8_symbol("module-creator"));
  scm_module_type =
    scm_make_foreign_object_type(name, slots, finalize_module);
#else
  // smob_id = scm_newsmob(&obj_funs);
  module_smob_id = scm_make_smob_type
    (const_cast<char*>("module"), sizeof(ModuleCreator));
  scm_set_smob_mark(module_smob_id, mark_object);
  scm_set_smob_print(module_smob_id, print_object);
  scm_set_smob_free(module_smob_id, free_object);
#endif
  scm_c_define_gsubr(s_make_module, 2, 0, 1, (scm_func) make_module);
  scm_c_define_gsubr(s_modify_module, 1, 0, 1, (scm_func) modify_module);
}



DUECA_NS_END

#include <undebprint.h>

#endif
