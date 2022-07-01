/* ------------------------------------------------------------------   */
/*      item            : TransportNotification.hxx
        made by         : Rene van Paassen
        date            : 010413
        category        : header file
        description     :
        changes         : 010413 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TransportNotification_hxx
#define TransportNotification_hxx

#ifdef TransportNotification_cxx
#endif

#include <Trigger.hxx>
#include <TransportClass.hxx>
#include <dueca_ns.h>
DUECA_NS_START

class UnifiedChannel;
class UChannelEntry;

/** A specific type of triggering used by channel ends.

    A TransportNotification is triggered when data has arrived in a
    channel and this data needs to be transported elsewhere. Upon
    triggering, the TransportNotification looks up its Packer -- the
    object responsible for packing the data for transport -- and tells
    it which data has to be packed.

    This class is for DUECA use only, not usable in application-level
    code */
class TransportNotification
{
  /** Channel I am notifying for. */
  UnifiedChannel* channel;

  /** The packer to call when something has to be packed. */
  GenericPacker* transporter;

  /** Each transporter has an index to the channel */
  int idx;

  /** Transport class */
  Channel::TransportClass tclass;

  /** Copying is not allowed, and not implemented. . */
  TransportNotification(const TransportNotification&);
public:

  /** Constructor */
  TransportNotification(GenericPacker* t, UnifiedChannel *c,
                        int idx, Channel::TransportClass tclass);

  /** Destructor */
  ~TransportNotification();

public:
  /** Return the channel that contains the data to be sent. */
  inline UnifiedChannel* getChannel() const {return channel;}

  /** Return the Packer that has to pack the data and set it on transport. */
  inline GenericPacker* getPacker() const {return transporter;}

  /** Return the class associated with this notification. */
  inline Channel::TransportClass getTransportClass() {return tclass; }

  /** The index. */
  inline int getIdx() const {return idx;}

  /** Trigger the packing of the data. */
  void trigger(UChannelEntry *entry, TimeTickType t);

  /** Print to stream, for debugging purposes. */
  friend ostream& operator << (ostream& s, const TransportNotification& o);
};

DUECA_NS_END
#endif
