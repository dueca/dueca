/* ------------------------------------------------------------------   */
/*      item            : StartIOStream.cxx
        made by         : Rene' van Paassen
        date            : 151023
        category        : body file
        description     :
        changes         : 151023 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define StartIOStream_cxx
#include "StartIOStream.hxx"
#include <iostream>
DUECA_NS_START;

void startIOStream()
{
  // see  https://gcc.gnu.org/bugzilla/show_bug.cgi?id=26123
  static std::ios_base::Init initIostreams;
}
DUECA_NS_END;
