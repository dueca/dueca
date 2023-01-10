/* ------------------------------------------------------------------   */
/*      item            : newlog_macros.hxx
        made by         : Rene van Paassen
        date            : 061117
        category        : header file
        description     :
        changes         : 061117 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifdef newlog_macros_hxx
#warning "Double inclusion of debug.h, move down in cxx files, use undebug.h in headers"
#else
#define newlog_macros_hxx

/** @file newlog-macros.hxx
    Definition of DUECA logging macros. */

#ifndef debug_h
#error "This file is not meant for direct inclusion; use through debug.h"
#endif

#ifdef D_CNF
#undef D_CNF
/// Debug messages for configuration initial state
#define D_CNF_INITIAL_ON true
#else
/// Debug messages for configuration initial state
#define D_CNF_INITIAL_ON false
#endif

/** @brief Debug messages for configuration related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_CNF(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,			\
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_cnf(), D_CNF_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_CNF
#undef I_CNF
/// Information messages for configuration initial state
#define I_CNF_INITIAL_ON true
#else
/// Information messages for configuration initial state
#define I_CNF_INITIAL_ON false
#endif

/** @brief Informational messages for configuration related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_CNF(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_cnf(), I_CNF_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_CNF
#undef W_CNF
/// Warning messages for configuration initial state
#define W_CNF_INITIAL_ON true
#else
/// Warning messages for configuration initial state
#define W_CNF_INITIAL_ON false
#endif

/** @brief Warning messages for configuration related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_CNF(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_cnf(), W_CNF_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_CNF

/** @brief Error messages for configuration related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_CNF(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_cnf());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

// SYS: system running messages
#ifdef D_SYS
#undef D_SYS
/// Debug messages for base system initial state
#define D_SYS_INITIAL_ON true
#else
/// Debug messages for base system initial state
#define D_SYS_INITIAL_ON false
#endif

/** @brief Debug messages for DUECA base system related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_SYS(A)                                                        \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_sys(), D_SYS_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_SYS
#undef I_SYS
/// Informational messages for base system initial state
#define I_SYS_INITIAL_ON true
#else
/// Informational messages for base system initial state
#define I_SYS_INITIAL_ON false
#endif

/** @brief Informational messages for DUECA base system related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_SYS(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_sys(), I_SYS_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_SYS
#undef W_SYS
/// Warning messages for base system initial state
#define W_SYS_INITIAL_ON true
#else
/// Warning messages for base system initial state
#define W_SYS_INITIAL_ON false
#endif

/** @brief Warning messages for DUECA base system related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_SYS(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_sys(), W_SYS_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_SYS

/** @brief Error messages for DUECA base system related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_SYS(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_sys());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// ACT: activation related messages
#ifdef D_ACT
#undef D_ACT
/// Debug messages for triggering and activation initial state
#define D_ACT_INITIAL_ON true
#else
/// Debug messages for triggering and activation initial state
#define D_ACT_INITIAL_ON false
#endif

/** @brief Debug messages for triggering&activation related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_ACT(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_act(), D_ACT_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_ACT
#undef I_ACT
/// Informational messages for triggering and activation initial state
#define I_ACT_INITIAL_ON true
#else
/// Informational messages for triggering and activation initial state
#define I_ACT_INITIAL_ON false
#endif

/** @brief Informational messages for triggering&activation related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_ACT(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_act(), I_ACT_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_ACT
#undef W_ACT
/// Warning messages for triggering and activation initial state
#define W_ACT_INITIAL_ON true
#else
/// Warning messages for triggering and activation initial state
#define W_ACT_INITIAL_ON false
#endif

/** @brief Warning messages for triggering&activation related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_ACT(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_act(), W_ACT_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_ACT

/** @brief Error messages for triggering&activation related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_ACT(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_act());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// CHN: channel related messages
#ifdef D_CHN
#undef D_CHN
/// Debug messages for channel communication initial state
#define D_CHN_INITIAL_ON true
#else
/// Debug messages for channel communication initial state
#define D_CHN_INITIAL_ON false
#endif

/** @brief Debug messages for channel communication related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_CHN(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_chn(), D_CHN_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_CHN
#undef I_CHN
/// Informational messages for channel communication initial state
#define I_CHN_INITIAL_ON true
#else
/// Informational messages for channel communication initial state
#define I_CHN_INITIAL_ON false
#endif

/** @brief Informational messages for channel communication related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_CHN(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_chn(), I_CHN_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_CHN
#undef W_CHN
/// Warning messages for channel communication initial state
#define W_CHN_INITIAL_ON true
#else
/// Warning messages for channel communication initial state
#define W_CHN_INITIAL_ON false
#endif

/** @brief Warning messages for channel communication related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_CHN(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_chn(), W_CHN_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_CHN

/** @brief Error messages for channel communication related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_CHN(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_chn());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// SHM: shared memory related messages
#ifdef D_SHM
#undef D_SHM
/// Debug messages for shared memory initial state
#define D_SHM_INITIAL_ON true
#else
/// Debug messages for shared memory initial state
#define D_SHM_INITIAL_ON false
#endif

/** @brief Debug messages for shared memory backend related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_SHM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_shm(), D_SHM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_SHM
#undef I_SHM
/// Informational messages for shared memory initial state
#define I_SHM_INITIAL_ON true
#else
/// Informational messages for shared memory initial state
#define I_SHM_INITIAL_ON false
#endif

/** @brief Informational messages for shared memory backend related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_SHM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_shm(), I_SHM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_SHM
#undef W_SHM
/// Warning messages for shared memory initial state
#define W_SHM_INITIAL_ON true
#else
/// Warning messages for shared memory initial state
#define W_SHM_INITIAL_ON false
#endif

/** @brief Warning messages for shared memory backend related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_SHM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_shm(), W_SHM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_SHM

/** @brief Error messages for shared memory backend related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_SHM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_shm());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// TIM: timing related messages
#ifdef D_TIM
#undef D_TIM
/// Debug messages for timing initial state
#define D_TIM_INITIAL_ON true
#else
/// Debug messages for timing initial state
#define D_TIM_INITIAL_ON false
#endif

/** @brief Debug messages for timing related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_TIM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_tim(), D_TIM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_TIM
#undef I_TIM
/// Informational messages for timing initial state
#define I_TIM_INITIAL_ON true
#else
/// Informational messages for timing initial state
#define I_TIM_INITIAL_ON false
#endif

/** @brief Informational messages for timing related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_TIM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_tim(), I_TIM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_TIM
#undef W_TIM
/// Warning messages for timing initial state
#define W_TIM_INITIAL_ON true
#else
/// Warning messages for timing initial state
#define W_TIM_INITIAL_ON false
#endif

/** @brief Warning messages for timing related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_TIM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_tim(), W_TIM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_TIM

/** @brief Error messages for timing related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_TIM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_tim());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// NET: network related messages
#ifdef D_NET
#undef D_NET
/// Debug messages for network communication initial state
#define D_NET_INITIAL_ON true
#else
/// Debug messages for network communication initial state
#define D_NET_INITIAL_ON false
#endif

/** @brief Debug messages for network backend related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_NET(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_net(), D_NET_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_NET
#undef I_NET
/// Informational messages for network communication initial state
#define I_NET_INITIAL_ON true
#else
/// Informational messages for network communication initial state
#define I_NET_INITIAL_ON false
#endif

/** @brief Informational messages for network backend related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_NET(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_net(), I_NET_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_NET
#undef W_NET
/// Warning messages for network communication initial state
#define W_NET_INITIAL_ON true
#else
/// Warning messages for network communication initial state
#define W_NET_INITIAL_ON false
#endif

/** @brief Warning messages for network backend related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_NET(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_net(), W_NET_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_NET

/** @brief Error messages for network backend related actions.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_NET(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_net());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// MOD: messages by application modules
#ifdef D_MOD
#undef D_MOD
/// Debug messages for base system active
#define D_MOD_ACTIVE
/// Debug messages for application code (modules) initial state
#define D_MOD_INITIAL_ON true
#else
/// Debug messages for application code (modules) initial state
#define D_MOD_INITIAL_ON false
#endif

/** @brief Debug messages for application module code.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_MOD(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_mod(), D_MOD_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_MOD
#undef I_MOD
/// Informational messages for base system active
#define I_MOD_ACTIVE
/// Informational messages for application code (modules) initial state
#define I_MOD_INITIAL_ON true
#else
/// Informational messages for application code (modules) initial state
#define I_MOD_INITIAL_ON false
#endif

/** @brief Informational messages for application module code.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_MOD(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_mod(), I_MOD_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_MOD
#undef W_MOD
/// Warning messages for application code (modules) initial state
#define W_MOD_INITIAL_ON true
#else
/// Warning messages for application code (modules) initial state
#define W_MOD_INITIAL_ON false
#endif

/** @brief Warning messages for application module code.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_MOD(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_mod(), W_MOD_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_MOD

/** @brief Error messages for application module code.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_MOD(A)                                                        \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_mod());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// STS: status monitoring related messages
#ifdef D_STS
#undef D_STS
/// Debug messages for state machine initial state
#define D_STS_INITIAL_ON true
#else
/// Debug messages for state machine initial state
#define D_STS_INITIAL_ON false
#endif

/** @brief Debug messages for distributed state machine.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_STS(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_sts(), D_STS_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_STS
#undef I_STS
/// Informational messages for state machine initial state
#define I_STS_INITIAL_ON true
#else
/// Informational messages for state machine initial state
#define I_STS_INITIAL_ON false
#endif

/** @brief Informational messages for distributed state machine.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_STS(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_sts(), I_STS_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_STS
#undef W_STS
/// Warning messages for state machine initial state
#define W_STS_INITIAL_ON true
#else
/// Warning messages for state machine initial state
#define W_STS_INITIAL_ON false
#endif

/** @brief Warning messages for distributed state machine.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_STS(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_sts(), W_STS_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_STS

/** @brief Error messages for distributed state machine.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_STS(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_sts());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// TRM: model trim related messages
#ifdef D_TRM
#undef D_TRM
/// Debug messages for trim calculation initial state
#define D_TRM_INITIAL_ON true
#else
/// Debug messages for trim calculation initial state
#define D_TRM_INITIAL_ON false
#endif

/** @brief Debug messages for trim calculation.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_TRM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_trm(), D_TRM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_TRM
#undef I_TRM
/// Informational messages for trim calculation initial state
#define I_TRM_INITIAL_ON true
#else
/// Informational messages for trim calculation initial state
#define I_TRM_INITIAL_ON false
#endif

/** @brief Informational messages for trim calculation.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_TRM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_trm(), I_TRM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_TRM
#undef W_TRM
/// Warning messages for trim calculation initial state
#define W_TRM_INITIAL_ON true
#else
/// Warning messages for trim calculation initial state
#define W_TRM_INITIAL_ON false
#endif

/** @brief Warning messages for trim calculation.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_TRM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_trm(), W_TRM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_TRM

/** @brief Error messages for trim calculation.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_TRM(A)                                                        \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_trm());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// MEM: memory related messages
#ifdef D_MEM
#undef D_MEM
/// Debug messages for memory handling initial state
#define D_MEM_INITIAL_ON true
#else
/// Debug messages for memory handling initial state
#define D_MEM_INITIAL_ON false
#endif

/** @brief Debug messages for memory management.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_MEM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_mem(), D_MEM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_MEM
#undef I_MEM
/// Informational messages for memory handling initial state
#define I_MEM_INITIAL_ON true
#else
/// Informational messages for memory handling initial state
#define I_MEM_INITIAL_ON false
#endif

/** @brief Informational messages for memory management.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_MEM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_mem(), I_MEM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_MEM
#undef W_MEM
/// Warning messages for memory handling initial state
#define W_MEM_INITIAL_ON true
#else
/// Warning messages for memory handling initial state
#define W_MEM_INITIAL_ON false
#endif

/** @brief Warning messages for memory management.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_MEM(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_mem(), W_MEM_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_MEM

/** @brief Error messages for memory management.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_MEM(A)                                                        \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_mem());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }


// INT: interconnect messages
#ifdef D_INT
#undef D_INT
/// Debug messages for DUECA interconnections initial state
#define D_INT_INITIAL_ON true
#else
/// Debug messages for DUECA interconnections initial state
#define D_INT_INITIAL_ON false
#endif

/** @brief Debug messages from the DUECA interconnector.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_INT(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_int(), D_INT_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_INT
#undef I_INT
/// Informational messages for DUECA interconnections initial state
#define I_INT_INITIAL_ON true
#else
/// Informational messages for DUECA interconnections initial state
#define I_INT_INITIAL_ON false
#endif

/** @brief Informational messages from the DUECA interconnector.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_INT(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_int(), I_INT_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_INT
#undef W_INT
/// Warning messages for DUECA interconnections initial state
#define W_INT_INITIAL_ON true
#else
/// Warning messages for DUECA interconnections initial state
#define W_INT_INITIAL_ON false
#endif

/** @brief Warning messages from the DUECA interconnector.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_INT(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_int(), W_INT_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_INT

/** @brief Error messages from the DUECA interconnector.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_INT(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_int());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

// XTR: extra component messages (hdf5 logger, extra)
#ifdef D_XTR
#undef D_XTR
/// Debug messages for extras initial state
#define D_XTR_INITIAL_ON true
#else
/// Debug messages for extras initial state
#define D_XTR_INITIAL_ON false
#endif

/** @brief Debug messages for extra facilities.
    @param A    Message to be printed, as std::ostream expression.
*/
#define D_XTR(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Debug),              \
        CCDUECA_NS::logcat_xtr(), D_XTR_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef I_XTR
#undef I_XTR
/// Informational messages for extras initial state
#define I_XTR_INITIAL_ON true
#else
/// Informational messages for extras initial state
#define I_XTR_INITIAL_ON false
#endif

/** @brief Informational messages for extra facilities.
    @param A    Message to be printed, as std::ostream expression.
*/
#define I_XTR(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Info),                \
        CCDUECA_NS::logcat_xtr(), I_XTR_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#ifdef W_XTR
#undef W_XTR
/// Warning messages for extras initial state
#define W_XTR_INITIAL_ON true
#else
/// Warning messages for extras initial state
#define W_XTR_INITIAL_ON false
#endif

/** @brief Warning messages for extra facilities.
    @param A    Message to be printed, as std::ostream expression.
*/
#define W_XTR(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
       CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Warning),             \
        CCDUECA_NS::logcat_xtr(), W_XTR_INITIAL_ON);                    \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#undef E_XTR

/** @brief Error messages for extra facilities.
    @param A    Message to be printed, as std::ostream expression.
*/
#define E_XTR(A) \
  { static CCDUECA_NS::Logger logger                                    \
      ( & __FILE__ [ SOURCE_PATH_SIZE ], __LINE__,                      \
        CCDUECA_NS::LogLevel(CCDUECA_NS::LogLevel::Error),              \
        CCDUECA_NS::logcat_xtr());                                      \
    if (logger) { logger << A << std::ends;                             \
      logger.transmit(); } }

#endif

