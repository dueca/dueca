/* ------------------------------------------------------------------   */
/*      item            : HDFLogConfig.hxx
        made by         : Rene van Paassen
        date            : 230710
        category        : header file
        description     : Compatibility header, change from HDFLogConfig
	                  to DUECALogConfig
        changes         : 230710 first version
        language        : C++
        copyright       : (c) 2023 René van Paassen
*/

#pragma once
#include <dueca/DUECALogConfig.hxx>
#warning "Please use DUECALogConfig instead of HDFLogConfig"

namespace dueca {
  typedef DUECALogConfig HDFLogConfig;
}
