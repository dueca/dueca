/* ------------------------------------------------------------------   */
/*      item            : ChannelDataMonitor.cxx
        made by         : Rene' van Paassen
        date            : 180519
        category        : body file
        description     :
        changes         : 180519 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ChannelDataMonitor_cxx
#include "ChannelDataMonitor.hxx"
#include <dueca/ChannelMonitorResult.hxx>

#include <debprint.h>

DUECA_NS_START

ChannelDataMonitor::ChannelDataMonitor(ChannelOverview *master,
                                       unsigned channelno, unsigned entryno) :
  master(master),
  channelno(channelno),
  entryno(entryno)
{}

ChannelDataMonitor::~ChannelDataMonitor()
{
  //
}

void ChannelDataMonitor::refreshData(const ChannelMonitorResult &rdata)
{
  DEB(rdata);
}
void ChannelDataMonitor::close() {}

void ChannelDataMonitor::open() {}

bool ChannelDataMonitor::isOpen() const { return false; }
DUECA_NS_END
