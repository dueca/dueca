/* ------------------------------------------------------------------   */
/*      item            : ChannelDef.hxx
        made by         : Rene van Paassen
        date            : 141015
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 141015 first version
                          160425 extended doc strings
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "ChannelDef.hxx"
#include "AmorphStore.hxx"

DUECA_NS_START;

static const char* getString(Channel::EntryTimeAspect e)
{
  static const char* names[] = {
    "Continuous",  "Events",  "AnyTimeAspect" };
  return names[unsigned(e)];
}

static const char* getString(Channel::EntryArity e)
{
  static const char* names[] = {
    "OnlyOneEntry", "ZeroOrOneEntries", "ZeroOrMoreEntries", "OneOrMoreEntries" };
  return names[unsigned(e)];
}

static const char* getString(Channel::ReadingMode e)
{
  static const char* names[] = {
    "OneOrMoreEntries", "ReadReservation", "JumpToMatchTime" };
  return names[unsigned(e)];
}

static const char* getString(Channel::PackingMode e)
{
  static const char* names[] = {
    "MixedPacking", "OnlyFullPacking"};
  return names[unsigned(e)];
}

static const char* getString(Channel::TransportClass e)
{
  static const char* names[] = {
    "UndefinedTransport", "Bulk", "Regular", "HighPriority"};
  return names[unsigned(e)];
}

template<>
const char* getclassname<dueca::Channel::EntryTimeAspect>()
{ return "EntryTimeAspect"; }
template<>
const char* getclassname<dueca::Channel::EntryArity>()
{ return "EntryArity"; }
template<>
const char* getclassname<dueca::Channel::PackingMode>()
{ return "PackingMode"; }
template<>
const char* getclassname<dueca::Channel::TransportClass>()
{ return "TransportClass"; }

DUECA_NS_END;


PRINT_NS_START;
std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::EntryTimeAspect& tc)
{ return os << getString(tc); }

std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::EntryArity& tc)
{ return os << getString(tc); }

std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::ReadingMode& tc)
{ return os << getString(tc); }

std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::PackingMode& tc)
{ return os << getString(tc); }

std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::TransportClass& tc)
{ return os << getString(tc); }

PRINT_NS_END;
