/* ------------------------------------------------------------------   */
/*      item            : DuecaNetPeer.hxx
        made by         : Rene van Paassen
        date            : 171225
        category        : header file
        description     :
        changes         : 171225 first version
        language        : C++
*/

#ifndef DuecaNetPeer_hxx
#define DuecaNetPeer_hxx

#include "NetCommunicatorPeer.hxx"
#include <dueca/ScriptCreatable.hxx>
#include <dueca/Accessor.hxx>

DUECA_NS_START;

class DuecaNetPeer:
  public Accessor,
  public NetCommunicatorPeer
{
  /** Define shorthand for class */
  typedef DuecaNetPeer _ThisClass_;

  /** A sequence number, to get unique names. */
  static int sequence;

  /** Priority of activity */
  PrioritySpec priority;

  /** Timing object */
  PeriodicTimeSpec time_spec;

  /** Minimum size of fill section */
  size_t fill_minimum;

  /** Stop command received */
  bool commanded_stop;

  /** Clock for starting timing */
  AperiodicAlarm clock;

  /** Callback */
  Callback<_ThisClass_> cb;

  /** Activity */
  ActivityCallback      net_io;

public:
  SCM_FEATURES_DEF;

  /** Constructor */
  DuecaNetPeer();

  /** Destructor */
  ~DuecaNetPeer();

  /** Complete creation */
  bool complete();

  /** Return table with configuration parameters */
  static const ParameterTable* getParameterTable();

  /** Object classification */
  ObjectType getObjectType() const {return O_CommAccessor;};

  /** Return one of the message buffers, needed for compatibility Unpackers */
  void returnBuffer(MessageBuffer::ptr_type buffer) final;

private:

  /** Main activity */
  void runIO(const TimeSpec& ts);

  /** Specify timing */
  bool setTimeSpec(const TimeSpec& ts);

  /** Master url */
  bool setMasterUrl(const std::string &url) { master_url = url; return true; }

private:

  /** decode configuration payload.

      Send order in message.

      @param s      Buffer with config data
      @param id     Id for the sending client
  */
  void clientDecodeConfig(AmorphReStore& s) final;

  /** encode configuration payload.

      node id and name
  */
  void clientSendConfig() final;

  /** send initial welcome message */
  void clientSendWelcome() final;

  /** Connection is established */
  void clientIsConnected() final;

  /** Pack payload into send buffer

      Uses the packers to fill the buffer. Packing only starts after all
      clients complete.

      @param buffer Payload buffer object
  */
  void clientPackPayload(MessageBuffer::ptr_type buffer) final;

  /** Accept a loaded buffer for unpacking

      This accepts a buffer and passes it on to the unpackers.

      @param buffer Payload buffer object
   */
  void clientUnpackPayload(MessageBuffer::ptr_type buffer, unsigned id,
                           TimeTickType current_tick, TimeTickType peertick,
                           int usecoffset) final;

  /** Stop the activity, at end of program. */
  void prepareToStop();

};

DUECA_NS_END;

#endif
