/* ------------------------------------------------------------------   */
/*      item            : @Module@.cxx
        made by         : @author@
        from template   : DuecaModuleTemplate.cxx (2022.06)
        date            : @date@
        category        : body file
        description     :
        changes         : @date@ first version
        language        : C++
        copyright       : (c)
*/


#define @Module@_cxx

// include the definition of the module class
#include "@Module@.hxx"

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dueca.h>

// include the debug writing header, by default, write warning and
// error messages
#define W_MOD
#define E_MOD
#include <debug.h>

// class/module name
const char* const @Module@::classname = "@smodule@";

// Parameters to be inserted
const ParameterTable* @Module@::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_,TimeSpec>
        (&_ThisModule_::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_,std::vector<int> >
      (&_ThisModule_::checkTiming), check_timing_description },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "please give a description of this module"} };

  return parameter_table;
}

// constructor
@Module@::@Module@(Entity* e, const char* part, const
                   PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process

  // initialize the channel access tokens, check the documentation for the
  // various parameters. Some examples:
  // r_mytoken(getId(), NameSet(getEntity(), getclassname<MyData>(), part),
  //           getclassname<MyData>(), 0, Channel::Events),
  // w_mytoken(getId(), NameSet(getEntity(), getclassname<MyData2>(), part),
  //           getclassname<MyData2>(), "label", Channel::Continuous),

  // create a clock, if you need time based triggering
  // instead of triggering on the incoming channels
  // myclock(),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::doCalculation),
  // the module's main activity
  do_calc(getId(), "@activityname@", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(/* fill in your triggering channels,
                        or enter the clock here */);
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

// as an example, the setTimeSpec function
bool @Module@::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);
  // or do this with the clock if you have it (don't do both!)
  // myclock.changePeriodAndOffset(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool @Module@::checkTiming(const std::vector<int>& i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

// tell DUECA you are prepared
bool @Module@::isPrepared()
{
  bool res = true;

  // Example checking a token:
  // CHECK_TOKEN(w_somedata);

  // Example checking anything
  // CHECK_CONDITION(myfile.good());
  // CHECK_CONDITION2(sometest, "some test failed");

  // return result of checks
  return res;
}

// start the module
void @Module@::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void @Module@::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void @Module@::doCalculation(const TimeSpec& ts)
{
  // access the input
  // example:
  // try {
  //   DataReader<MyData> u(r_mytoken, ts);
  //   throttle = u.data().throttle;
  //   de = u.data().de; ....
  // }
  // catch(Exception& e) {
  //   // strange, there is no input. Should I try to continue or not?
  // }
  /* The above piece of code shows a block in which you try to catch
     error conditions (exceptions) to handle the case in which the input
     data is lost. This is not always necessary, if you normally do not
     foresee such a condition, and you don t mind being stopped when
     it happens, forget about the try/catch blocks. */

  // do the simulation or other calculations, one step

  // DUECA applications are data-driven. From the time a module is switched
  // on, it should produce data, so that modules "downstream" are
  // activated
  // access your output channel(s)
  // example
  // DataWriter<MyData2> y(w_mytoken, ts);

  // write the output into the output channel, using the data writer
  // y.data().var1 = something; ...
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
static TypeCreator<@Module@> a(@Module@::getMyParameterTable());

