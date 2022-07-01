/* ------------------------------------------------------------------   */
/*      item            : GenericCallback.cxx
        made by         : Rene' van Paassen
        date            : 980609
        category        : body file
        description     : Function object, to be derived for various
                          callback types.
        copyright       : (c) 1998 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2")
*/


#define GenericCallback_cc
#include "GenericCallback.hxx"
using namespace std;

DUECA_NS_START

GenericCallback::GenericCallback()
{
  // nothing
}

GenericCallback::~GenericCallback()
{
  // nothing
}

ostream& operator << (ostream& os, const GenericCallback& callback)
{
  callback.print (os);
  return os;
}

DUECA_NS_END

