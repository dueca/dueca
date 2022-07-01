/* ------------------------------------------------------------------   */
/*      item            : Entity_Scheme.hxx
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

#ifndef Entity_Scheme_hxx
#define Entity_Scheme_hxx

#include <Entity.hxx>
#include <CreationCenter.hxx>
#include <SchemeClassData.hxx>
#include <dueca/SchemeObject.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

//SCM_FEATURES_IMP(Entity,"entity");

template<>
SchemeClassData<Entity>* SchemeClassData<Entity>::single()
{
  static SchemeClassData<Entity> singleton
    ("entity", SchemeClassData<ScriptCreatable>::single());
  return &singleton;
}

// define here
bool CreationCenter_validTag(SCM i);

//SCM_FEATURES_IMP(Entity,"entity");

static SCM make_object(SCM name, SCM arglist)
{
  DUECA_SCM_ASSERT
    (SCM_NIMP(name) && scm_is_string(name), name, SCM_ARG1,
     SchemeClassData<Entity>::single()->getMakeName());
  DUECA_SCM_ASSERT
    (SCM_NIMP(arglist) && SCM_CONSP(arglist), arglist, SCM_ARG2,
     SchemeClassData<Entity>::single()->getMakeName());

  // create an empty (C++) list of modulecreator objects
  list<ModuleCreator*> foetae;
  // and an empty list of scheme objects that must be kept alive
  list<SchemeObject*> refs;

  // the list should only contain lists of modules, or empty
  // lists, or SCM_UNSPECIFIED values, those result from an "imploded"
  // if statement
  // walk through the arguments, they should all be lists of lists of
  // modules. NOTE: have to add some more rigorous checking!
  while (arglist != SCM_EOL) {

    // check a sublist
    SCM sublist = SCM_CAR(arglist);
    while (sublist != SCM_UNSPECIFIED && sublist != SCM_EOL) {

      // check that this entry is a pair, and that its data points to
      // a module(creator) object
      DUECA_SCM_ASSERT(SCM_NIMP(sublist) && SCM_CONSP(sublist),
                       sublist, SCM_ARGn, SchemeClassData<Entity>::single()->getMakeName());
      DUECA_SCM_ASSERT(SCM_NIMP(SCM_CAR(sublist)) &&
                       CreationCenter_validTag(SCM_CAR(sublist)),
                       SCM_CAR(sublist), SCM_ARGn,
                       SchemeClassData<Entity>::single()->getMakeName());

      // extract the wrapping SchemeObject
#if defined(SCM_USE_FOREIGN)
      // if this is true, then add this Module to the list
      SchemeObject* obj =
        reinterpret_cast<SchemeObject*>
        (scm_foreign_object_ref(SCM_CAR(sublist), size_t(0)));
#else
      // extract the wrapping SchemeObject
      SchemeObject* obj =
        reinterpret_cast<SchemeObject*>(SCM_SMOB_DATA(SCM_CAR(sublist)));
#endif
      ModuleCreator *mc = obj->getModuleCreator();
      assert(mc != NULL);

      // collect into list
      foetae.push_back(mc);
      refs.push_back(obj);

      // move to the next list element
      sublist = SCM_CDR(sublist);
    }
    arglist = SCM_CDR(arglist);

    // check whether we're still dealing with a pair, or with the
    // empty list object
    DUECA_SCM_ASSERT(arglist == SCM_EOL ||
                     (SCM_NIMP(arglist) && SCM_CONSP(arglist)),
                     arglist, SCM_ARGn,
                     SchemeClassData<Entity>::single()->getMakeName());
  }

  // now if there are any modules to be created in this node
  if (foetae.size()) {

    Entity* ent = new Entity(dueca_scm_chars(name), foetae);

#if defined(SCM_USE_FOREIGN)
    SchemeObject *obj =
      new (scm_gc_malloc(sizeof(SchemeObject), "entity"))
      SchemeObject(ent);
    SCM fob = scm_make_foreign_object_1
      (SCM_PACK(SchemeClassData<Entity>::single()->getTag()), obj);
#else
    SchemeObject *obj = new SchemeObject(ent);
    SCM fob = dueca_new_smob
      (SchemeClassData<Entity>::single()->getTag(), obj);
#endif

    // remember scheme object
    obj->setSCM(fob);

    // link any supplied modules
    for (list<SchemeObject*>::iterator ii = refs.begin();
         ii!= refs.end(); ii++) {
      obj->addReferred((*ii)->getSCM());
    }
    return fob;
  }
  else {

    // no need to create an entity object. Tell the EntityManager
    // however that his work (non-existent) is done
    // EntityManager::single()->noCheckIn(dueca_scm_chars(name));
  }

  // return some value
  return SCM_BOOL_T;
}

#if defined(SCM_USE_FOREIGN)
static void finalize_entity(SCM smob)
{
  DEB("finalizing entity");
  SchemeObject* obj = reinterpret_cast<SchemeObject*>
    (scm_foreign_object_ref(smob, size_t(0)));

  // explicitly call destructor, scheme will clean up memory
  obj->~SchemeObject();
}

#else
static SCM mark_entity(SCM smob)
{
  reinterpret_cast<SchemeObject*>
    (SCM_SMOB_DATA(smob))->markReferred();
  return SCM_BOOL_F;
}

static scm_sizet free_entity(SCM smob)
{
  SchemeObject* obj =
    reinterpret_cast<SchemeObject*>(SCM_SMOB_DATA(smob));
  delete obj; // scoped pointer will ensure delete of encapsulated C++
  return 0;
}

static int print_entity(SCM smob, SCM port, scm_print_state *pstate)
{
  //SchemeObject* sobj =
  //  reinterpret_cast<SchemeObject*>(SCM_SMOB_DATA(smob));
  //Entity* obj = dynamic_cast<Entity*>(sobj->getObject());
  scm_puts(const_cast<char*>("#<Entity>"), port);
  return 1;
}
#endif

template<>
void scheme_init<Entity>()
{
#if !defined(SCM_USE_FOREIGN)
  SchemeClassData<Entity>::single()->setTag
    (scm_make_smob_type(SchemeClassData<Entity>::single()->getName(),
                        sizeof(Entity*)));
  scm_set_smob_mark(SchemeClassData<Entity>::single()->getTag(), mark_entity);
  scm_set_smob_print(SchemeClassData<Entity>::single()->getTag(), print_entity);
  scm_set_smob_free(SchemeClassData<Entity>::single()->getTag(), free_entity);
#else
  SCM name = scm_from_utf8_symbol(SchemeClassData<Entity>::single()->getName());
  SCM slots = scm_list_1(scm_from_utf8_symbol("entity"));
  SchemeClassData<Entity>::single()->setTag
    (SCM_UNPACK(scm_make_foreign_object_type(name, slots, finalize_entity)));
#endif
  scm_c_define_gsubr(SchemeClassData<Entity>::single()->getMakeName(),
                     1, 0, 1, (scm_func) make_object);
}

DUECA_NS_END

#include <undebprint.h>

#endif
