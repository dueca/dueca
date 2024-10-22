/* ------------------------------------------------------------------   */
/*      item            : UCallbackOrActivity.cxx
        made by         : Rene' van Paassen
        date            : 240910
        category        : body file
        description     :
        changes         : 240910 first version
        language        : C++
        copyright       : (c) 24 TUDelft-AE-C&S
*/

#include "UCallbackOrActivity.hxx"
#include "Trigger.hxx"
#include <dueca/Activity.hxx>
#include <dueca/GenericCallback.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START;

UCallbackOrActivity::UCallbackOrActivity(Activity *act) :
  TriggerPuller("channel validity"),
  act(act),
  cb(NULL)
{
  //
}

UCallbackOrActivity::UCallbackOrActivity(GenericCallback *cb) :
  act(NULL),
  cb(cb)
{}

UCallbackOrActivity::UCallbackOrActivity(std::nullptr_t ncb) :
  act(NULL),
  cb(NULL)
{}

UCallbackOrActivity::UCallbackOrActivity() :
  act(NULL),
  cb(NULL)
{}

UCallbackOrActivity::UCallbackOrActivity(const UCallbackOrActivity &o) :
  act(o.act),
  cb(o.cb)
{}

UCallbackOrActivity::~UCallbackOrActivity()
{
  if (cb || act) {
    DEB("Callback or activity never called");
  }
}

void UCallbackOrActivity::operator()(const TimeSpec &ts)
{
  if (cb) {
    (*cb)(ts);
    cb = NULL;
  }
  else if (act) {
    act->setTrigger(*this);
    TriggerPuller::pull(ts);
    act = NULL;
  }
}

DUECA_NS_END;