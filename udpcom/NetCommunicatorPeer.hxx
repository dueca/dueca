/* ------------------------------------------------------------------   */
/*      item            : NetCommunicatorPeer.hxx
        made by         : Rene van Paassen
        date            : 170912
        category        : header file
        description     :
        changes         : 170912 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef NetCommunicatorPeer_hxx
#define NetCommunicatorPeer_hxx

#include "NetCommunicator.hxx"
#include "ConfigBuffer.hxx"
#include "WebsockCommunicator.hxx"
#include <dueca/CommonCallback.hxx>
#include <dueca/AsyncList.hxx>
#include <UDPPeerConfig.hxx>
#include <limits>

#include <Ticker.hxx>

/** Generic TCP/IP + packet (UDP or websocket) communication base
    class.

    This implements a peer/slave system to communicate with a a master
    controlling the sending and communication process.

    Derived classes need to implement functions for the following:

    clientDecodeConfig - decode client-specific communication data from the master

    clientSendConfig -   send client-specific configuration requests to
                         the master, run regularly

    clientSendWelcome -  send initial welcome request with needed
                         information on the peer.

    clientIsConnected -  information on when the client is connected

    Communication on the sending process is handled by the
    NetCommunicatorPeer class itself. The callback functions only add
    possibilities for client configuration.
 */
class NetCommunicatorPeer: public NetCommunicator
{
private:
  /** restrict these */
  using NetCommunicator::codeAndSendUDPMessage;

protected:
  /** @defgroup configurationpeer Configuration value for peer
      @{ */

  /** Master url, websocket URL for contacting the master. */
  std::string                         master_url;

  /** Data url override, for cases where port mapping alters the data
      port */
  std::string                         override_data_url;
  /** @} */

  /** connection to master for configuration */
  boost::intrusive_ptr<WebsockCommunicatorPeerConfig>
                                      conf_comm;

private:

  /** Communication handling buffer */
  ConfigBuffer                        commbuf;

  /** Preceding id, one to react to with UDP messages */
  uint16_t                            follow_id;

  /** Last communication cycle */
  uint32_t                            last_cycle;

  /** Flag to report stop */
  volatile bool                       stop_commanded;

  /** List with changes in follow id */
  AsyncList<UDPPeerConfig>            follow_changes;

  /** Connection established flag */
  bool                                connection;

  /** UDP cycle tracking flag */
  bool                                trackingudpcycle;

  /** Tick time */
  TimeTickType                        current_tick;

  /** Node ID claimed by the incoming message */
  uint16_t                            i_nodeid;

  /** New number of peers */
  uint16_t                            lastround_npeers;

  /** Time to send */
  bool                                myturntosend;

protected:
  /** Constructor */
  NetCommunicatorPeer();

  /** Destructor */
  ~NetCommunicatorPeer();

private:
  /** timed read on the config socket */
  unsigned readConfigSocket(bool wait);

  /** Get the config message and pass these */
  void receiveConfigMessage(MessageBuffer::ptr_type& buffer);

  /** decode configuration socket for additional data */
  bool decodeConfigData();

  /** setup connection */
  void setupConnection(Activity& activity);

  /** Do one loop cycle

      @param act    activity, used for signaling blocking actions */
  void _oneCycle(Activity& act);

  /** Unpack the data, this function is used as callback by the
      data_comm object. */
  void unpackPeerData(MessageBuffer::ptr_type& buffer);

  /** Send any planned changes (leaving), and client data across */
  void peerSendConfig();

protected:
  /** @defgroup clientcalls Calls for using this class, except for data pack/unpack,
      defined in NetCommunication.hxx

     @{
  */

  /** Send additional config to master.

      Send the configuration over the tcp link.

      @param s      Buffer with config data to send.
   */
  void sendConfig(AmorphStore &s);

  /** decode configuration messages.

      Passes application-dependent configuration data. Note that
      configuration data is passed as single messages. Decodes one
      object or message; when decode fails, pass the associated
      exception, so the decoding can be stopped and resumed when
      additional data has been received.

      @param s       Store with config data

      @throws AmorphReStoreEmpty Throwing of this exception flags
                     incomplete client data in the buffer (i.e., the
                     socket needs to be read further before the
                     information is complete).
  */
  virtual void clientDecodeConfig(AmorphReStore& s) = 0;

  /** encode configuration payload.

      Encode multiple objects or messages. Encode each object preceded
      by a UDPPeerConfig::ClientPayload message. Send with one or more calls to
      the sendConfig method.
  */
  virtual void clientSendConfig() = 0;

  /** send initial welcome message */
  virtual void clientSendWelcome();

  /** Inform that the connection is established */
  virtual void clientIsConnected() = 0;

  /** reset configuration of the client */
  virtual void resetClientConfiguration();

  /** Decide on continuation of blocking calls

      @param last_tick Tick time for stopping
  */
  void setStopTime(const TimeTickType& last_tick);

  /** Do one loop cycle

      @param act    activity, used for signaling blocking actions */
  void oneCycle(Activity& act);

  /** Enter blocking loop

      Blocking loop, sets up TCP connection, then UDP connection. Claims the
      thread, can be ended using setStopTime.

      @param act    activity, used for signaling blocking actions
   */
  void startCyclic(Activity& act);

  /** Break the connections */
  void clearConnections();

  /** @} */
};

#endif

/* Send state logic:

   Loop structure for a peer

   * - block with select on receive port
   * - read message
       ? small messages are ignored; continue
   * - header is decoded
       ? own message are ignored; continue
       ? if the sender is 0
       * -





   Normal: cycle entered when master index is larger than previous one
   - Buffers are swapped
   - message cycle from master is assumed for the buffer cycle
   -
*/
