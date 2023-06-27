/*      item            : HDFLogStatus.hxx
        made by         : Rene van Paassen
        date            : 230710
        category        : header file
        description     : Compatibility header, change from HDFLogStatus
	                  to DUECALogStatus
        changes         : 230710 first version
        language        : C++
        copyright       : (c) 2023 René van Paassen
*/

#pragma once
#include <dueca/DUECALogStatus.hxx>
#warning "Please use DUECALogStatus instead of HDFLogStatus"

namespace dueca {
  typedef DUECALogStatus HDFLogStatus;
}
