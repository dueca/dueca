/* ------------------------------------------------------------------   */
/*      item            : NetCommunicatorExceptions.cxx
        made by         : Rene' van Paassen
        date            : 170217
        category        : body file
        description     :
        changes         : 170217 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define NetCommunicatorExceptions_cxx
#include "NetCommunicatorExceptions.hxx"

DUECA_NS_START;

const char* configconnectionbroken::what() const throw()
{
  return "tcp connection broken";
}

configconnectionbroken::configconnectionbroken() :
  std::exception()
{
  //
}

const char* connectionfails::what() const throw()
{
  return "Cannot set-up network connection";
}

connectionfails::connectionfails():
  std::exception()
{
  //
}

const char* packetcommunicationfailure::what() const throw()
{
  return reason;
}

packetcommunicationfailure::packetcommunicationfailure(const char* reason):
  std::exception(),
  reason(reason)
{
  //
}



DUECA_NS_END;

