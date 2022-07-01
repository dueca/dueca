/* ------------------------------------------------------------------   */
/*      item            : ReplicatorNamespace.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Mon Jan 30 21:42:36 2017
        category        : header file
        description     :
        changes         : Mon Jan 30 21:42:36 2017 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReplicatorNamespace_hxx
#define ReplicatorNamespace_hxx

#include <dueca-conf.h>

#define STARTNSREPLICATOR namespace dueca {
#define ENDNSREPLICATOR };

STARTNSREPLICATOR;
class ChannelReplicator;
class EntryWatcher;
class EntryReader;
class EntryWriter;

// failure probability for testing robustness of the comm
#ifdef BUILD_TESTOPT
// probability of an elementary failures
static const double test_failprob = 0.0001;
#endif

ENDNSREPLICATOR;

//#define REP_DEBUG(A) std::cerr << A << std::endl;
#ifndef REP_DEBUG
#define REP_DEBUG(A)
#endif

#endif
