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

/** @file Definition of DUECA logging macros */

#ifndef debug_h
#error "This file is not meant for direct inclusion; use through debug.h"
#endif

#ifdef D_CNF
#undef D_CNF
#define D_CNF_ACTIVE
#define D_CNF_INITIAL_ON true
#else
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
#define I_CNF_ACTIVE
#define I_CNF_INITIAL_ON true
#else
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
#define W_CNF_ACTIVE
#define W_CNF_INITIAL_ON true
#else
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
#define E_CNF_ACTIVE


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
#define D_SYS_ACTIVE
#define D_SYS_INITIAL_ON true
#else
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
#define I_SYS_ACTIVE
#define I_SYS_INITIAL_ON true
#else
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
#define W_SYS_ACTIVE
#define W_SYS_INITIAL_ON true
#else
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
#define E_SYS_ACTIVE

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
#define D_ACT_ACTIVE
#define D_ACT_INITIAL_ON true
#else
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
#define I_ACT_ACTIVE
#define I_ACT_INITIAL_ON true
#else
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
#define W_ACT_ACTIVE
#define W_ACT_INITIAL_ON true
#else
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
#define E_ACT_ACTIVE

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
#define D_CHN_ACTIVE
#define D_CHN_INITIAL_ON true
#else
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
#define I_CHN_ACTIVE
#define I_CHN_INITIAL_ON true
#else
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
#define W_CHN_ACTIVE
#define W_CHN_INITIAL_ON true
#else
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
#define E_CHN_ACTIVE

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
#define D_SHM_ACTIVE
#define D_SHM_INITIAL_ON true
#else
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
#define I_SHM_ACTIVE
#define I_SHM_INITIAL_ON true
#else
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
#define W_SHM_ACTIVE
#define W_SHM_INITIAL_ON true
#else
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
#define E_SHM_ACTIVE

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
#define D_TIM_ACTIVE
#define D_TIM_INITIAL_ON true
#else
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
#define I_TIM_ACTIVE
#define I_TIM_INITIAL_ON true
#else
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
#define W_TIM_ACTIVE
#define W_TIM_INITIAL_ON true
#else
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
#define E_TIM_ACTIVE

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
#define D_NET_ACTIVE
#define D_NET_INITIAL_ON true
#else
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
#define I_NET_ACTIVE
#define I_NET_INITIAL_ON true
#else
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
#define W_NET_ACTIVE
#define W_NET_INITIAL_ON true
#else
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
#define E_NET_ACTIVE

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
#define D_MOD_ACTIVE
#define D_MOD_INITIAL_ON true
#else
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
#define I_MOD_ACTIVE
#define I_MOD_INITIAL_ON true
#else
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
#define W_MOD_ACTIVE
#define W_MOD_INITIAL_ON true
#else
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
#define E_MOD_ACTIVE

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
#define D_STS_ACTIVE
#define D_STS_INITIAL_ON true
#else
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
#define I_STS_ACTIVE
#define I_STS_INITIAL_ON true
#else
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
#define W_STS_ACTIVE
#define W_STS_INITIAL_ON true
#else
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
#define E_STS_ACTIVE

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
#define D_TRM_ACTIVE
#define D_TRM_INITIAL_ON true
#else
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
#define I_TRM_ACTIVE
#define I_TRM_INITIAL_ON true
#else
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
#define W_TRM_ACTIVE
#define W_TRM_INITIAL_ON true
#else
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
#define E_TRM_ACTIVE

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
#define D_MEM_ACTIVE
#define D_MEM_INITIAL_ON true
#else
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
#define I_MEM_ACTIVE
#define I_MEM_INITIAL_ON true
#else
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
#define W_MEM_ACTIVE
#define W_MEM_INITIAL_ON true
#else
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
#define E_MEM_ACTIVE

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
#define D_INT_ACTIVE
#define D_INT_INITIAL_ON true
#else
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
#define I_INT_ACTIVE
#define I_INT_INITIAL_ON true
#else
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
#define W_INT_ACTIVE
#define W_INT_INITIAL_ON true
#else
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
#define E_INT_ACTIVE

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
#define D_XTR_ACTIVE
#define D_XTR_INITIAL_ON true
#else
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
#define I_XTR_ACTIVE
#define I_XTR_INITIAL_ON true
#else
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
#define W_XTR_ACTIVE
#define W_XTR_INITIAL_ON true
#else
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
#define E_XTR_ACTIVE

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

