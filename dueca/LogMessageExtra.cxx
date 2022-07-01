/* ------------------------------------------------------------------   */
/*      item            : LogMessageExtra.cxx
        made by         : Rene van Paassen
        date            : 070720
        category        : header file
        description     : additional methods LogMessage.dco
        changes         : 070720 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

DUECA_NS_END
#include <LogPoints.hxx>
#include <ActivityDescriptions.hxx>
#include <iomanip>
#include <sstream>
DUECA_NS_START

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

void LogMessage::printNice(std::ostream& os) const
{
  this->time.show(os);
    union { uint32_t i; char name[5]; } catconv; catconv.name[4] = '\000';
  const LogPoint &point = LogPoints::single().getPoint
    (this->logpoint, this->context.parts.node);
  ostringstream owner;
  owner << ActivityDescriptions::single()[this->context].owner;

  catconv.i = point.category;
  os << setw(6) << this->count
     << setw(2) << point.level << setw(3) << catconv.name
     << setw(33) << point.fname << ':'
     << setw(4) << setfill('0') << point.line << setfill(' ')
     << setw(3) << this->context.parts.node
     << setw(3) << this->context.parts.manager
     << setw(9) << owner.str()
     << setw(33) << ActivityDescriptions::single()[this->context].name
     << '|' << this->message << std::endl;
}

// verify the size of LogMessage
struct __LogMessageSizeCheck {
  __LogMessageSizeCheck() {
    assert(sizeof(LogMessage) == 256);
  }
};

static struct __LogMessageSizeCheck onecheck __attribute__((unused));

#if 0
#undef DOBS
#define DOBS(A) std::cerr << A << std::endl;

#define __CUSTOM_FUNCTION_UNPACKDATA
void LogMessage::unPackData(::dueca::AmorphReStore& s)
{
  DOBS("unPackData LogMessage, size" << sizeof(logpoint) + sizeof(count) +
       sizeof(time) + sizeof(context) + message.size()+1 << ':' << message);

  ::unPackData(s, this->logpoint);
  ::unPackData(s, this->count);
  ::unPackData(s, this->time);
  ::unPackData(s, this->context);
  ::unPackData(s, this->message);

}

#define __CUSTOM_FUNCTION_PACKDATA
void LogMessage::packData(::dueca::AmorphStore& s) const
{
  DOBS("packData LogMessage, size" << sizeof(logpoint) + sizeof(count) +
       sizeof(time) + sizeof(context) + message.size()+1);
  ::packData(s, this->logpoint);
  ::packData(s, this->count);
  ::packData(s, this->time);
  ::packData(s, this->context);
  ::packData(s, this->message);
}

#define __CUSTOM_AMORPHRESTORE_CONSTRUCTOR
LogMessage::LogMessage(AmorphReStore& s):
        logpoint(s),
        count(s),
        time(s),
        context(s),
        message(s)
{
  DOBS("Amorph Constructor LogMessage, size" << sizeof(logpoint) + sizeof(count) +
       sizeof(time) + sizeof(context) + message.size()+1 << ':' << message);
  //DOBS("amorph constructor LogMessage");
}

#undef DOBS
#define DOBS(A)
#endif
