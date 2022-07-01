## -*-python-*-
## dueca.cnf: created with DUECA version @dueca-version@
## Created on: @date@

### import the dueca namespace; provided by the c++ code
import dueca

### parameters defining cooperation with other nodes
this_node_id = 0                       # id of the current node
no_of_nodes = 1                        # total number of nodes used
send_order = 0                         # order/prio in send cycle

### parameter defining real_time behaviour
highest_manager = 3                    # max priority of activities
run_in_multiple_threads = True         # test with #f with threading problems
rt_sync_mode = 2                       # 0=sigwait, obsolete
                                       # 1=select, portable, obsolete
                                       # 2=nanosleep, good for all modern
                                       #   Linux kernels,
                                       #   slaves as well as masters
                                       # 3=rtc, obsolete

# graphic interface selection, typically "none", "gtk2", "gtk3"
graphic_interface = "gtk3"             # selection of interface

### parameters defining "size" of the time. Note that all nodes should have
### the same compatible_increment, and for all nodes 
### tick_time_step/tick_base_increment should be the same
tick_base_increment = 100              # logical increment of time, each tick
tick_compatible_increment = 100        # same, but used at start_up
tick_time_step = 0.01                  # time step for each tick
communication_interval = 100           # interval for initiating comm

### parameter for communication using multicast
if_address = "127.0.0.1"        # address of own ip interface
mc_address = "224.0.0.1"        # multicast address
master_address = "netmaster"    # hostname or address comm master
mc_port = 7500                  # master control port
packet_size = 4096              # size of packets
bulk_max_size = 128*1024        # max size of bulk messages
comm_prio_level = 2             # priority communication process
unpack_prio_level = 1           # priority unpacking incoming data
bulk_unpack_prio_level = 1      # priority unpacking bulk data

### choice for the communication. 
use_ip_comm = no_of_nodes > 1          # if true, use ethernet

### ___________________________________________________________________

###  1 _ ObjectManager. This enables named objects to be created,
###      and allows query of the node id and number of nodes
DUECA_objectmanager = dueca.ObjectManager(
    this_node_id, no_of_nodes).complete()

###  2 _ the environment. The environment will create the necessary
###      number of activity managers, so activities may now be
###      scheduled. From this point on it is also possible to create
###      activities
DUECA_environment = dueca.Environment().param(
    multi_thread = run_in_multiple_threads,
    highest_priority = highest_manager,
    graphic_interface = graphic_interface,
    command_interval = 0.25,
    command_lead = 0.25).complete()

### 2c _ now priority specs can be made
comm_prio = dueca.PrioritySpec(comm_prio_level, 0)
unpack_prio = dueca.PrioritySpec(unpack_prio_level, 0)
bulk_unpack_prio = dueca.PrioritySpec(bulk_unpack_prio_level, 0)

###  3 _ Packers, and a packer manager. Packers are passive
###      objects, accessed by the channels, and provide the configuration
###      data for remote communication. The unpackers use an
###	 activity, and therefore must start after the environment
if use_ip_comm:
    DUECA_packer = dueca.Packer().complete()
    DUECA_unpacker = dueca.Unpacker().param(
        priority_spec = unpack_prio).complete()
    DUECA_fillpacker = dueca.FillPacker().param(
        buffer_size = bulk_max_size).complete()
    DUECA_fillunpacker = dueca.FillUnpacker().param(
        priority_spec = bulk_unpack_prio,
        buffer_size = bulk_max_size).complete()

### the packer manager keeps an inventory of all packers for transport to 
### other nodes. The three arguments are a fill (bulk) packer, a normal packer
### and (if possible) a high_priority packer. One set per destination, here
### all referring to the same two packers
DUECA_packermanager = dueca.PackerManager()
if use_ip_comm:
    for i in range(no_of_nodes):
        DUECA_packermanager.param(
            add_set = dueca.PackerSet(
                DUECA_fillpacker, DUECA_packer, DUECA_packer))
DUECA_packermanager.complete()

###  4 _ The channel manager. From now on channel_using objects can
###      be created.
DUECA_channelmanager = dueca.ChannelManager().complete()

###  5 _ The ticker. A channel_using object! From now on
###      ticker_using objects can be created
DUECA_ticker = dueca.Ticker().param(
    base_increment = tick_base_increment,
    compatible_increment = tick_compatible_increment,
    time_step = tick_time_step,
    sync_mode = rt_sync_mode).complete()

###  6 _ communication hardware accessors. These may use the ticker
###      or channels to trigger activity.
if use_ip_comm and send_order == 0:

    # if a peer list has been created, follow it, otherwise invent a send
    # order
    if not locals().has_key('peerlist'):
        peerlist = [ p for p in range(no_of_nodes) if p != this_node_id ]

    # create master communicator, (optionally add port_reuse = True)
    DUECA_netcomm = dueca.NetMaster().param(
        packer = DUECA_packer,
        unpacker = DUECA_unpacker,
        fill_packer = DUECA_fillpacker,
        fill_unpacker = DUECA_fillunpacker,
        if_address = if_address,
        setup_port = mc_port,
        data_address = mc_address,
        data_port = mc_port + 1,
        timeout = 0.2,   
        packet_size = packet_size,
        set_priority = dueca.PrioritySpec(comm_prio_level, 0),
        set_timing = dueca.TimeSpec(0, communication_interval),
        node_list = peerlist).complete()
    del peerlist

if use_ip_comm and send_order != 0:

    # create peer communicator
    DUECA_netcomm = dueca.NetPeer().param(
        packer = DUECA_packer,
        unpacker = DUECA_unpacker,
        fill_packer = DUECA_fillpacker,
        fill_unpacker = DUECA_fillunpacker,
        if_address = if_address,
        setup_port = mc_port,
        master_address = master_address,
        set_priority = dueca.PrioritySpec(comm_prio_level, 0),
        set_timing = dueca.TimeSpec(0, communication_interval)).complete()
    
###  7   Pass control to the environment again.
###      It will now invoke a completeCreation method
###      from the previously created singletons (1, 3, 4, 5, 6) to 
###      give these the opportunity to do additional initialisation
###      Then it creates the EntityManager for this node, and the
###      configuration continues with dueca_mod.py

