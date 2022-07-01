/* ------------------------------------------------------------------   */
/*      item            : @Module@.cxx
        made by         : @author@
        from template   : DuecaHelperTemplate.cxx (2022.06)
        date            : @date@
        category        : body file
        description     :
        changes         : @date@ first version
        language        : C++
        copyright       : (c)
*/


#define @Module@_cxx
// include the definition of the helper class
#include "@Module@.hxx"

// include additional files needed for your calculation here

#define DO_INSTANTIATE
#include <VarProbe.hxx>
#include <MemberCall.hxx>
#include <MemberCall2Way.hxx>
#include <CoreCreator.hxx>

// include the debug writing header. Warning and error messages
// are on by default, debug and info can be selected by
// uncommenting the respective defines
//#define D_MOD
//#define I_MOD
#include <debug.h>

USING_DUECA_NS;

// Parameters to be inserted
const ParameterTable* @Module@::getParameterTable()
{
  static const ParameterTable parameter_table[] = {

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string). */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "please give a description of this helper class"} };

  return parameter_table;
}

// constructor
@Module@::@Module@() :
  ScriptCreatable()
{

}

bool @Module@::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
@Module@::~@Module@()
{
  //
}

// script access macro
SCM_FEATURES_NOIMPINH(@Module@, ScriptCreatable, "@smodule@");

// Make a CoreCreator object for this module, the CoreCreator
// will check in with the scheme-interpreting code, and enable the
// creation of objects of this type
static CoreCreator<@Module@> a(@Module@::getParameterTable());

