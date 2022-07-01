// -*-c++-*-
/**
\page log Log message generation and control

An overview of the facilities for creating log messages.

\section log_macros Logging macros, logging classes

In order to display and record log messages on significant events in
DUECA programs, a set of log macros is defined. To the programmer,
these macros present an easy way of printing a message; by simply
following the conventions for printing in C++. These messages are both
printed on the standard error output of a DUECA program, but they are
also sent, collected and recorded centrally within the DUECA
process. Each message is also coded with file name and line within the
file, so that a programmer can quickly find where the message was
generated.

The following logging classes are defined:

- Informational logging, mainly for debugging purposes. This is
  sometimes useful in developing a program, but in general generating
  these messages should be avoided in production. The macros are
  prefixed by `D_`.

- Informational logging, for occasional events. This may be useful to
  clarify events in the program. The macros are prefixed by `I_`.

- Warning logging, indicating something that should not be fatal to
  the rest of the DUECA process, but may involve degraded
  functionality (e.g. a network interface is not yet ready, and the
  system will try again). These messages are prefixed by `W_`.

- Error messages, serious enough to stop execution of the program, or
  abort part of the functionality (e.g., a module does not get
  created). These messages are prefixed by `E_`.

In addition, a number of categories are defined. The normal category
used by application programmers is `MOD`, indicating messages from a
DUECA module. DUECA itself distinguishes the following:

- `CNF`, this relates to configuration errors or warnings in the
  `dueca.cnf` / `dueca_cnf.py` files or `dueca.mod` / `dueca_mod.py`
  files.

- `ACT`, this pertains to messages for DUECA's activation and
  triggering system.

- `CHN`, for messages on the communication channel system.

- `SHM`, for messages on shared-memory (like SCRAMNet)
  communication. Note that this type of communication is now obsolete,
  since Ethernet-based communication software and hardware are both
  cheaper and more reliable.

- `TIM`, for messages on handling timing, e.g., information on jitter,
  latency, and synchronization of DUECA processes.

- `NET`, for information from (TCP/IP) network communication back-end
  code.

- `STS`, for information on the status of distributed state machines
  in DUECA and DUSIME.

- `TRM`, for information from the trim calculation code in DUSIME.

- `MEM`, for information on memory handling.

- `INT`, for information from the DUECA interconnect system for
  connecting different DUECA processes in distributed simulation
  exercises.

- `XTR`, for messages from extra support code in dueca-extra.

A log command would look like a normal function call, as an example,
here a message from the ChannelReplicator in the `dueca-inter` library:

@code
      /* DUECA interconnect.

         There is a difference in definition of a DCO data class
         between the current node and a remote node. Fix the code,
         probably by running an update and recompile, ensure the DCO
         definitions are identical. */
      E_INT("data class magic for " << *ci << " differs with node " << node); 
@endcode

\section log_control Switching logging on and off

Logging macros are accessible by including DUECA's `debug.h` header:

@code
#include <debug.h>
@endcode

By default, warning and error level messages are switched on, but debug and information level messages are suppressed. This means that the code to generate the message is inserted in a DUECA program, but by default "hitting" a logpoint will not generate a message. When developing, it is often useful to have more log information. You can select the default state of logging by providing C defines for the logging macros, which will be replaced by the `debug.h` header, for example:

@code
#define D_MOD
#define I_MOD
#include <debug.h>
@endcode

Now the `D_MOD` and `I_MOD` macros are defined to produce logging (as before), but they will by default be "active" for this source file, i.e., produce logging messages rather than be silent. Using the logging interface (see next section), the logging levels of the different log categories can be overridden during runtime.

\section log_output Logging output.

The log output is available in a number of different places:

  - In the standard error output of the DUECA program where the log message is generated.

  - In the file called dueca.messages, output by the node 0 DUECA program. In this file, messages from all dueca nodes are assembled. Additional information is available, such as the file name and line number of the error message, and the time is printed. Note that before sending, error messages may be throttled, to prevent clogging the communication channels. Each error message has a count indicating how many times the message has been hit, inspect that count to detect throttling. If available, there is also information on the ActivityManager (thread) associated with the message and the DUECA entity or module that was being executed.

  - Through the interface window that can be opened by selecting "View", "Error Log View" from DUECA's standard interface. This shows the same information as the dueca.messages file. The second tab in this interface allows the selection of the log level for each of the log classes, adjusting the generation of log messages for a running program.

Here is a @ref loglist "list of all log messages" in DUECA's libraries.

*/
