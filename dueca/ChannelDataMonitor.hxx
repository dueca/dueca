/* ------------------------------------------------------------------   */
/*      item            : ChannelDataMonitor.hxx
        made by         : Rene van Paassen
        date            : 180519
        category        : header file
        description     :
        changes         : 180519 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelDataMonitor_hxx
#define ChannelDataMonitor_hxx

#include <dueca_ns.h>

DUECA_NS_START


class ChannelOverview;
struct ChannelMonitorResult;


/** Base class for data view. */
class ChannelDataMonitor
{
protected:
  /** Pointer to the managing object */
  ChannelOverview  *master;

  /** Channel number */
  unsigned channelno;

  /** Entry number */
  unsigned entryno;

public:
  /** Constructor */
  ChannelDataMonitor(ChannelOverview  *master,
                     unsigned channelno,
                     unsigned entryno);

  /** Destructor */
  virtual ~ChannelDataMonitor();

  /** New data from the handler */
  virtual void refreshData(const ChannelMonitorResult& rdata);

    /** close the window */
  virtual void close();

  /** open the window */
  virtual void open();

};

DUECA_NS_END

#endif
