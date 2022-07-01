/* ------------------------------------------------------------------   */
/*      item            : ChannelWriteInfoExtra.cxx
        made by         : Rene' van Paassen
        date            : 180329
        category        : additional code DCO object
        description     : line-oriented print of channel write info
        changes         : 180329 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

DUECA_NS_END;

#include <iomanip>

DUECA_NS_START;

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

#define __CUSTOM_FUNCTION_PRINT
std::ostream & ChannelWriteInfo::print(std::ostream& s) const
{
  s << "channel (" << std::setw(2) << int(channelid.location) << ','
    << std::setw(4) << int(channelid.object)
    << "), client (" << std::setw(2) << int(clientid.location) << ','
    << std::setw(4) << int(clientid.object)
    << "), entry # " << std::setw(4)
    << entryid << " type " << setw(30) << dataclass
    << " l: " << setw(30) << label;
  return s;
}

void ChannelWriteInfo::printhead(std::ostream& s)
{
  s << "   chanid  clientid   entry_#  event                      dataclass label:"
    << std::endl;
}

void ChannelWriteInfo::printline(std::ostream& s) const
{
  s << std::setw(9) << channelid.object << ' '
    << std::setw(9) << clientid.printid() << ' '
    << std::setw(9) << entryid << ' '
    << std::setw(6) << eventtype << ' '
    << std::setw(30) << dataclass << " \""
    << label << '"' << std::endl;
}

