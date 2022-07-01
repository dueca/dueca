/* ------------------------------------------------------------------   */
/*      item            : Writer.cxx
        made by         : repa
	from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Tue Mar 27 11:53:59 2018
	category        : body file 
        description     : 
	changes         : Tue Mar 27 11:53:59 2018 first version
	template changes: 030401 RvP Added template creation comment
	                  060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Writer_cxx

// include the definition of the module class
#include "Writer.hxx"

// include the debug writing header, by default, write warning and 
// error messages
#define W_MOD
#define E_MOD
#include <debug.h>

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dueca.h>

// class/module name
const char* const Writer::classname = "writer";

// Parameters to be inserted
const ParameterTable* Writer::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing", 
      new MemberCall<_ThisModule_,TimeSpec>
        (&_ThisModule_::setTimeSpec), set_timing_description },

    { "check-timing", 
      new MemberCall<_ThisModule_,vector<int> >
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
Writer::Writer(Entity* e, const char* part, const
		   PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class. 
     You always pass the pointer to the entity, give the classname and the 
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  counter(0),
  // initialize the channel access tokens
  w_mytoken(getId(), NameSet(getEntity(), TestObject::classname, part),
            TestObject::classname, "label", Channel::Continuous),
  
  // create a clock, if you need time based triggering
  // instead of triggering on the incoming channels
  myclock(),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::doCalculation),
  // the module's main activity
  do_calc(getId(), "write some data", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(myclock);
}

bool Writer::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
Writer::~Writer()
{
  //
}

// as an example, the setTimeSpec function
bool Writer::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  // do_calc.setTimeSpec(ts);
  // or do this with the clock if you have it (don't do both!)
  myclock.changePeriodAndOffset(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool Writer::checkTiming(const vector<int>& i)
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
bool Writer::isPrepared()
{
  bool res = true;

  // Example checking a token:
  CHECK_TOKEN(w_mytoken);

  // return result of checks
  return res;
}

// start the module
void Writer::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void Writer::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You 
// should read the input channels here, and calculate and write the 
// appropriate output
void Writer::doCalculation(const TimeSpec& ts)
{
  DataWriter<TestObject> y(w_mytoken, ts);
  y.data().counter = counter;
} 

STARTUPSECTION(Writer)
// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
static TypeCreator<Writer> a(Writer::getMyParameterTable());
ENDSTARTUPSECTION
