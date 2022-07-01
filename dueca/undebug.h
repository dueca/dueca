/* ------------------------------------------------------------------   */
/*      item            : undebug.h
        made by         : Rene' van Paassen
        date            : long ago
        category        : header file
        description     :
        changes         : 20010801 Rvp update
        language        : C++
        documentation   : DUECA_API
        copyright       : (c) 2001 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** @file undebug.h

    Clear debugging macros. Only needed at the end of a header file
    that uses debugging macros. */

#ifndef debug_h
#error "Only include after having included debug.h"
#endif

#ifdef debug_h_double
#warning "Please move your debug.h inclusion further down"
#undef debug_h_double
#else

#ifdef newlog_macros_hxx
#undef newlog_macros_hxx

#undef D_CNF
#undef D_CNF_ACTIVE
#undef D_CNF_INITIAL_ON

#undef I_CNF
#undef I_CNF_ACTIVE
#undef I_CNF_INITIAL_ON

#undef W_CNF
#undef W_CNF_ACTIVE
#undef W_CNF_INITIAL_ON

#undef E_CNF
#undef E_CNF_ACTIVE

#undef D_SYS
#undef D_SYS_ACTIVE
#undef D_SYS_INITIAL_ON

#undef I_SYS
#undef I_SYS_ACTIVE
#undef I_SYS_INITIAL_ON

#undef W_SYS
#undef W_SYS_ACTIVE
#undef W_SYS_INITIAL_ON

#undef E_SYS
#undef E_SYS_ACTIVE

#undef D_ACT
#undef D_ACT_ACTIVE
#undef D_ACT_INITIAL_ON

#undef I_ACT
#undef I_ACT_ACTIVE
#undef I_ACT_INITIAL_ON

#undef W_ACT
#undef W_ACT_ACTIVE
#undef W_ACT_INITIAL_ON

#undef E_ACT
#undef E_ACT_ACTIVE

#undef D_CHN
#undef D_CHN_ACTIVE
#undef D_CHN_INITIAL_ON

#undef I_CHN
#undef I_CHN_ACTIVE
#undef I_CHN_INITIAL_ON

#undef W_CHN
#undef W_CHN_ACTIVE
#undef W_CHN_INITIAL_ON

#undef E_CHN
#undef E_CHN_ACTIVE

#undef D_SHM
#undef D_SHM_ACTIVE
#undef D_SHM_INITIAL_ON

#undef I_SHM
#undef I_SHM_ACTIVE
#undef I_SHM_INITIAL_ON

#undef W_SHM
#undef W_SHM_ACTIVE
#undef W_SHM_INITIAL_ON

#undef E_SHM
#undef E_SHM_ACTIVE

#undef D_TIM
#undef D_TIM_ACTIVE
#undef D_TIM_INITIAL_ON

#undef I_TIM
#undef I_TIM_ACTIVE
#undef I_TIM_INITIAL_ON

#undef W_TIM
#undef W_TIM_ACTIVE
#undef W_TIM_INITIAL_ON

#undef E_TIM
#undef E_TIM_ACTIVE

#undef D_NET
#undef D_NET_ACTIVE
#undef D_NET_INITIAL_ON

#undef I_NET
#undef I_NET_ACTIVE
#undef I_NET_INITIAL_ON

#undef W_NET
#undef W_NET_ACTIVE
#undef W_NET_INITIAL_ON

#undef E_NET
#undef E_NET_ACTIVE

#undef D_MOD
#undef D_MOD_ACTIVE
#undef D_MOD_INITIAL_ON

#undef I_MOD
#undef I_MOD_ACTIVE
#undef I_MOD_INITIAL_ON

#undef W_MOD
#undef W_MOD_ACTIVE
#undef W_MOD_INITIAL_ON

#undef E_MOD
#undef E_MOD_ACTIVE

#undef D_STS
#undef D_STS_ACTIVE
#undef D_STS_INITIAL_ON

#undef I_STS
#undef I_STS_ACTIVE
#undef I_STS_INITIAL_ON

#undef W_STS
#undef W_STS_ACTIVE
#undef W_STS_INITIAL_ON

#undef E_STS
#undef E_STS_ACTIVE

#undef D_TRM
#undef D_TRM_ACTIVE
#undef D_TRM_INITIAL_ON

#undef I_TRM
#undef I_TRM_ACTIVE
#undef I_TRM_INITIAL_ON

#undef W_TRM
#undef W_TRM_ACTIVE
#undef W_TRM_INITIAL_ON

#undef E_TRM
#undef E_TRM_ACTIVE

#undef D_MEM
#undef D_MEM_ACTIVE
#undef D_MEM_INITIAL_ON

#undef I_MEM
#undef I_MEM_ACTIVE
#undef I_MEM_INITIAL_ON

#undef W_MEM
#undef W_MEM_ACTIVE
#undef W_MEM_INITIAL_ON

#undef E_MEM
#undef E_MEM_ACTIVE

#undef D_INT
#undef D_INT_ACTIVE
#undef D_INT_INITIAL_ON

#undef I_INT
#undef I_INT_ACTIVE
#undef I_INT_INITIAL_ON

#undef W_INT
#undef W_INT_ACTIVE
#undef W_INT_INITIAL_ON

#undef E_INT
#undef E_INT_ACTIVE

#undef D_XTR
#undef D_XTR_ACTIVE
#undef D_XTR_INITIAL_ON

#undef I_XTR
#undef I_XTR_ACTIVE
#undef I_XTR_INITIAL_ON

#undef W_XTR
#undef W_XTR_ACTIVE
#undef W_XTR_INITIAL_ON

#undef E_XTR
#undef E_XTR_ACTIVE
#endif
#endif
