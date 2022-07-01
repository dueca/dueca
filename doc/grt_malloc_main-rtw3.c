/* $Revision: 4851 $
 * Copyright 1994-2001 The MathWorks, Inc.
 *
 * File    : grt_malloc_main.c
 *
 * Abstract:
 *      A Generic "Real-Time (single tasking or pseudo-multitasking,
 *      dynamically allocated data)" main that runs under most
 *      operating systems.
 *
 *      This file may be a useful starting point when targeting a new
 *      processor or microcontroller.
 *
 *
 * Compiler specified defines:
 *        RT              - Required.
 *      MODEL=modelname - Required.
 *        NUMST=#         - Required. Number of sample times.
 *        NCSTATES=#      - Required. Number of continuous states.
 *      TID01EQ=1 or 0  - Optional. Only define to 1 if sample time task
 *                        id's 0 and 1 have equal rates.
 *      MULTITASKING    - Optional. (use MT for a synonym).
 *        SAVEFILE        - Optional (non-quoted) name of .mat file to create.
 *                          Default is <MODEL>.mat
 *      BORLAND         - Required if using Borland C/C++
 */


#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tmwtypes.h"
#include "simstruc.h"
#include "rt_sim.h"
#include "rtwlog.h"
#include "rt_nonfinite.h"

/* Signal Handler header */
#ifdef BORLAND
#include <signal.h>
#include <float.h>
#endif

/* External Mode header */
#ifdef EXT_MODE
#include "updown.h"
#include "ext_svr.h"
#include "ext_share.h"
#include "ext_svr_transport.h"
#endif


/*=========*
 * Defines *
 *=========*/

#ifndef TRUE
#define FALSE (0)
#define TRUE  (1)
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#endif

#define QUOTE1(name) #name
#define QUOTE(name) QUOTE1(name)    /* need to expand name    */

#ifndef RT
# error "must define RT"
#endif

#ifndef MODEL
# error "must define MODEL"
#endif

#ifndef NUMST
# error "must define number of sample times, NUMST"
#endif

#ifndef NCSTATES
# error "must define NCSTATES"
#endif

#ifndef RT_MALLOC
# error "grt_malloc_main.c require RT_MALLOC to be defined"
#endif

#ifndef SAVEFILE
# define MATFILE2(file) #file ".mat"
# define MATFILE1(file) MATFILE2(file)
# define MATFILE MATFILE1(MODEL)
#else
# define MATFILE QUOTE(SAVEFILE)
#endif

#define RUN_FOREVER -1.0

/*====================*
 * External functions *
 *====================*/
extern SimStruct *MODEL(void);

#if NCSTATES > 0
  extern void rt_CreateIntegrationData(SimStruct *S);
  extern void rt_UpdateContinuousStates(SimStruct *S);
# if defined(RT_MALLOC)
   extern void rt_DestroyIntegrationData(SimStruct *S);
# endif
#else
# define rt_CreateIntegrationData(S)  ssSetSolverName(S,"FixedStepDiscrete");
# define rt_UpdateContinuousStates(S) ssSetT(S,ssGetSolverStopTime(S));
#endif


/*=============*
 * Global data *
 *=============*/

const char *RT_MEMORY_ALLOCATION_ERROR = "memory allocation error";

#ifdef EXT_MODE
    int_T volatile           startModel      = FALSE;
    TargetSimStatus volatile modelStatus     = TARGET_STATUS_WAITING_TO_START;
#endif

/*==================================*
 * Global data local to this module *
 *==================================*/

static struct {
  int_T    stopExecutionFlag;
  int_T    isrOverrun;
  int_T    overrunFlags[NUMST];
  const char_T *errmsg;
} GBLbuf;



/*=================*
 * Local functions *
 *=================*/

#ifdef BORLAND
/* Implemented for BC++ only*/

typedef void (*fptr)(int, int);

/* Function: divideByZero =====================================================
 *
 * Abstract: Traps the error Division by zero and prints a warning
 *           Also catches other FP errors, but does not identify them
 *           specifically.
 */
void divideByZero(int sigName, int sigType)
{
    signal(SIGFPE, (fptr)divideByZero);
    if ((sigType == FPE_ZERODIVIDE)||(sigType == FPE_INTDIV0)){
        printf("*** Warning: Division by zero\n\n");
        return;
    }
    else{
        printf("*** Warning: Floating Point error\n\n");
        return;
    }
} /* end divideByZero */

#endif /* BORLAND */

#if !defined(MULTITASKING)  /* SINGLETASKING */

/* Function: rtOneStep ========================================================
 *
 * Abstract:
 *      Perform one step of the model. This function is modeled such that
 *      it could be called from an interrupt service routine (ISR) with minor
 *      modifications.
 */
static void rt_OneStep(SimStruct *S)
{
    real_T tnext;

    /***********************************************
     * Check and see if base step time is too fast *
     ***********************************************/

    if (GBLbuf.isrOverrun++) {
        GBLbuf.stopExecutionFlag = 1;
        return;
    }

    /***********************************************
     * Check and see if error status has been set  *
     ***********************************************/

    if (ssGetErrorStatus(S) != NULL) {
        GBLbuf.stopExecutionFlag = 1;
        return;
    }

    /* enable interrupts here */

#ifdef EXT_MODE
    /*
     * In a multi-tasking environment, this would be removed from the base rate
     * and called as a "background" task.
     */
    rt_MsgServerWork(S);
    rt_UploadServerWork(S);
#endif

    tnext = rt_GetNextSampleHit(S);
    ssSetSolverStopTime(S,tnext);

    sfcnOutputs(S,0);

#ifdef EXT_MODE
    UploadCheckTrigger();

#   if NUMST==1
        UploadBufAddTimePoint(S, 0);
#   else
    {   /*
         * This is a single tasking simulation with multiple rates.  The real
         * time simulation handles all rates in one task, but Simulink assigns
         * a seperate tid to each rate.  Since external mode is tid-based, each
         * tid must be checked for uploaded data.  It is recommended that
         * multi-tasking mode be used for models with more than one rate.
         */
        int i;
        for (i=0; i<NUMST; i++) {
            if (ssIsSampleHit(S, i, unused)) {
                UploadBufAddTimePoint(S, i);
            }
        }
    }
#   endif
#endif

    GBLbuf.errmsg = rt_UpdateTXYLogVars(S);
    if (GBLbuf.errmsg != NULL) {
        GBLbuf.stopExecutionFlag = 1;
        return;
    }

    sfcnUpdate(S,0);

    rt_UpdateDiscreteTaskSampleHits(S);

    if (ssGetSampleTime(S,0) == CONTINUOUS_SAMPLE_TIME) {
        rt_UpdateContinuousStates(S);
    }

    GBLbuf.isrOverrun--;

#ifdef EXT_MODE
    UploadCheckEndTrigger();
#endif
} /* end rtOneStep */

#else /* MULTITASKING */

# if TID01EQ == 1
#  define FIRST_TID 1
# else
#  define FIRST_TID 0
# endif

/* Function: rtOneStep ========================================================
 *
 * Abstract:
 *      Perform one step of the model. This function is modeled such that
 *      it could be called from an interrupt service routine (ISR) with minor
 *      modifications.
 *
 *      This routine is modeled for use in a multitasking environment and
 *        therefore needs to be fully re-entrant when it is called from an
 *        interrupt service routine.
 *
 * Note:
 *      Error checking is provided which will only be used if this routine
 *      is attached to an interrupt.
 *
 */
static void rt_OneStep(SimStruct *S)
{
    int_T  eventFlags[NUMST];
    int_T  i;
    real_T tnext;
    int_T  *sampleHit = ssGetSampleHitPtr(S);

    /***********************************************
     * Check and see if base step time is too fast *
     ***********************************************/

    if (GBLbuf.isrOverrun++) {
        GBLbuf.stopExecutionFlag = 1;
        return;
    }

    /***********************************************
     * Check and see if error status has been set  *
     ***********************************************/

    if (ssGetErrorStatus(S) != NULL) {
        GBLbuf.stopExecutionFlag = 1;
        return;
    }
    /* enable interrupts here */

#ifdef EXT_MODE
    /*
     * In a multi-tasking environment, this would be removed from the base rate
     * and called as a "background" task.
     */
    rt_MsgServerWork(S);
    rt_UploadServerWork(S);
#endif

    /************************************************************************
     * Update discrete events and buffer event flags locally so that ISR is *
     * re-entrant.                                                          *
     ************************************************************************/

    tnext = rt_UpdateDiscreteEvents(S);
    ssSetSolverStopTime(S,tnext);
    for (i=FIRST_TID+1; i < NUMST; i++) {
        eventFlags[i] = sampleHit[i];
    }


    /*******************************************
     * Step the model for the base sample time *
     *******************************************/

    sfcnOutputs(S,FIRST_TID);

#ifdef EXT_MODE
    UploadCheckTrigger();
    UploadBufAddTimePoint(S, FIRST_TID);
#endif

    GBLbuf.errmsg = rt_UpdateTXYLogVars(S);
    if (GBLbuf.errmsg != NULL) {
        GBLbuf.stopExecutionFlag = 1;
        return;
    }

    sfcnUpdate(S,FIRST_TID);

    if (ssGetSampleTime(S,0) == CONTINUOUS_SAMPLE_TIME) {
        rt_UpdateContinuousStates(S);
    }
     else {
        rt_UpdateDiscreteTaskTime(S,0);
    }

#if FIRST_TID == 1
    rt_UpdateDiscreteTaskTime(S,1);
#endif


    /************************************************************************
     * Model step complete for base sample time, now it is okay to          *
     * re-interrupt this ISR.                                               *
     ************************************************************************/

    GBLbuf.isrOverrun--;


    /*********************************************
     * Step the model for any other sample times *
     *********************************************/

    for (i=FIRST_TID+1; i<NUMST; i++) {
        if (eventFlags[i]) {
            if (GBLbuf.overrunFlags[i]++) {  /* Are we sampling too fast for */
                GBLbuf.stopExecutionFlag=1;  /*   sample time "i"?           */
                return;
            }

            sfcnOutputs(S,i);
#ifdef EXT_MODE
            UploadBufAddTimePoint(S, i);
#endif
            sfcnUpdate(S,i);

            rt_UpdateDiscreteTaskTime(S,i);

            /* Indicate task complete for sample time "i" */
            GBLbuf.overrunFlags[i]--;
        }
    }

#ifdef EXT_MODE
    UploadCheckEndTrigger();
#endif
} /* end rtOneStep */

#endif /* MULTITASKING */


/* Function ====================================================================
 * Pause the process (w/o hogging the cpu) until receive start message
 * from the host.
 */
#ifdef EXT_MODE
static void WaitForStartMsgFcn(SimStruct *S)
{
    while(!startModel) {
        grt_Sleep(0L, 375000L);
        rt_MsgServerWork(S);
        rt_UploadServerWork(S);
    }
} /* end WaitForStartMsgFcn */
#endif


/* Function ====================================================================
 * Pause the process (w/o hogging the cpu) until receive step message (which
 * means the startModel flag moves to true) or until we are no longer
 * in the paused state.  The message/upload server must continue to process
 * events (otherwise the host would not be able to communicate with the target).
 */
#ifdef EXT_MODE
static void PauseIfNeeded(SimStruct *S)
{
    while((modelStatus == TARGET_STATUS_PAUSED) &&
          !startModel &&
          !ssGetStopRequested(S)) {
        grt_Sleep(0L, 375000L);
        rt_MsgServerWork(S);
        rt_UploadServerWork(S);
    }
    startModel = FALSE; /* reset to FALSE - if we were stepped we want to
                         *                  stop again next time we get
                         *                  back here.
                         */
} /* end WaitForStartMsgFcn */
#endif

static void displayUsage (void)
{
    (void) printf("usage: %s [finaltime] [TCPport]\n",QUOTE(MODEL));
    (void) printf("arguments:\n");
    (void) printf("  finaltime - overrides final time specified in "
                  "Simulink (inf for no limit)\n");
    (void) printf("  ExternModeTCPport - overrides 17725 default port, "
                  "valid range 256 to 65535\n");
}

/*===================*
 * Visible functions *
 *===================*/


/* Function: main =============================================================
 *
 * Abstract:
 *      Execute model on a generic target such as a workstation.
 */
int_T main(int_T argc, const char_T *argv[])
{
    SimStruct  *S;
    const char *status;
    real_T     finaltime = -2.0;

    int_T  oldStyle_argc;
    const char_T *oldStyle_argv[5];

    /******************************
     * MathError Handling for BC++ *
     ******************************/
#ifdef BORLAND
    signal(SIGFPE, (fptr)divideByZero);
#endif

    /*******************
     * Parse arguments *
     *******************/

    if ((argc > 1) && (argv[1][0] != '-')) {
        /* old style */
        if ( argc > 3 ) {
            displayUsage();
            exit(EXIT_FAILURE);
        }

        oldStyle_argc    = 1;
        oldStyle_argv[0] = argv[0];

        if (argc >= 2) {
            oldStyle_argc = 3;

            oldStyle_argv[1] = "-tf";
            oldStyle_argv[2] = argv[1];
        }

        if (argc == 3) {
            oldStyle_argc = 5;

            oldStyle_argv[3] = "-port";
            oldStyle_argv[4] = argv[2];

        }

        argc = oldStyle_argc;
        argv = oldStyle_argv;

    }

    {
        /* new style: */
        double    tmpDouble;
        char_T str2[200];
        int_T  count      = 1;
        int_T  parseError = FALSE;

        /*
         * Parse the standard RTW parameters.  Let all unrecognized parameters
         * pass through to external mode for parsing.  NULL out all args handled
         * so that the external mode parsing can ignore them.
         */
        while(count < argc) {
            const char_T *option = argv[count++];

            /* final time */
            if ((strcmp(option, "-tf") == 0) && (count != argc)) {
                const char_T *tfStr = argv[count++];

                sscanf(tfStr, "%200s", str2);
                if (strcmp(str2, "inf") == 0) {
                    tmpDouble = RUN_FOREVER;
                } else {
                    char_T tmpstr[2];

                    if ( (sscanf(str2, "%lf%1s", &tmpDouble, tmpstr) != 1) ||
                         (tmpDouble < 0.0) ) {
                        (void)printf("finaltime must be a positive, real value or inf\n");
                        parseError = TRUE;
                        break;
                    }
                }
                finaltime = (real_T) tmpDouble;

                argv[count-2] = NULL;
                argv[count-1] = NULL;
            }
        }

        if (parseError) {
            (void)printf("\nUsage: %s -option1 val1 -option2 val2 -option3 "
                         "...\n\n", QUOTE(MODEL));
            (void)printf("\t-tf 20 - sets final time to 20 seconds\n");

            exit(EXIT_FAILURE);
        }

#ifdef EXT_MODE
        {
            const char_T *extParseErrorMsg = ExtParseArgsAndInitUD(argc, argv);
            if (extParseErrorMsg != NULL) {
                printf(
                    "\nError processing External Mode command line arguments:\n");
                printf("\t%s",extParseErrorMsg);

                exit(EXIT_FAILURE);
            }
        }
#endif

        /*
         * Check for unprocessed ("unhandled") args.
         */
        {
            int i;
            for (i=1; i<argc; i++) {
                if (argv[i] != NULL) {
                    printf("Unexpected command line argument: %s\n",argv[i]);
                }
            }
        }
    }

    /****************************
     * Initialize global memory *
     ****************************/
    (void)memset(&GBLbuf, 0, sizeof(GBLbuf));

    /************************
     * Initialize the model *
     ************************/
    rt_InitInfAndNaN(sizeof(real_T));

    S = MODEL();
    if (S == NULL) {
        (void)fprintf(stderr,"Memory allocation error during model "
                      "registration");
        exit(EXIT_FAILURE);
    }
    if (ssGetErrorStatus(S) != NULL) {
        (void)fprintf(stderr,"Error during model registration: %s\n",
                      ssGetErrorStatus(S));
        sfcnTerminate(S);
        exit(EXIT_FAILURE);
    }
    if (finaltime >= 0.0 || finaltime == RUN_FOREVER) ssSetTFinal(S,finaltime);

    sfcnInitializeSizes(S);
    sfcnInitializeSampleTimes(S);
    if ((status=rt_InitTimingEngine(S)) != NULL) {
        (void)fprintf(stderr,
                "Failed to initialize sample time engine: %s\n", status);
        exit(EXIT_FAILURE);
    }
    rt_CreateIntegrationData(S);
#if NCSTATES > 0
    if(ssGetErrorStatus(S) != NULL) {
        (void)fprintf(stderr, "Error creating integration data.\n");
        rt_DestroyIntegrationData(S);
        sfcnTerminate(S);
        exit(EXIT_FAILURE);
    }
#endif

    GBLbuf.errmsg = rt_StartDataLogging(S);
    if (GBLbuf.errmsg != NULL) {
        (void)fprintf(stderr,"Error starting data logging: %s\n",GBLbuf.errmsg);
        return(EXIT_FAILURE);
    }

#ifdef EXT_MODE
    if (rt_ExtModeInit() != EXT_NO_ERROR) return(EXIT_FAILURE);

    /*
     * Pause until receive model start message.
     */
    if (ExtWaitForStartMsg()) {
        WaitForStartMsgFcn(S);
    }
    if (modelStatus != TARGET_STATUS_PAUSED) {
        modelStatus = TARGET_STATUS_RUNNING;
    } else {
        /* leave in pause mode */
    }
#endif

    (void)printf("\n** starting the model **\n");

    sfcnStart(S);
    if (ssGetErrorStatus(S) != NULL) {
      GBLbuf.stopExecutionFlag = 1;
    }

    /*************************************************************************
     * Execute the model.  You may attach rtOneStep to an ISR, if so replace *
     * the call to rtOneStep (below) with a call to a background task        *
     * application.                                                          *
     *************************************************************************/

    if (ssGetTFinal(S) == RUN_FOREVER) {
        printf ("\n**May run forever. Model stop time set to infinity.**\n");
    }

    while (!GBLbuf.stopExecutionFlag &&
           (ssGetTFinal(S) == RUN_FOREVER ||
            ssGetTFinal(S)-ssGetT(S) > ssGetT(S)*DBL_EPSILON)) {

#ifdef EXT_MODE
        if (modelStatus == TARGET_STATUS_PAUSED) {
            PauseIfNeeded(S);
        }
#endif

        if (ssGetStopRequested(S)) break;
        rt_OneStep(S);
    }

    if (!GBLbuf.stopExecutionFlag && !ssGetStopRequested(S)) {
        /* Execute model last time step */
        rt_OneStep(S);
    }

    /********************
     * Cleanup and exit *
     ********************/

    rt_StopDataLogging(MATFILE,S);

#ifdef EXT_MODE
    rt_ExtModeShutdown();
#endif

    if (GBLbuf.errmsg) {
        (void)fprintf(stderr,"%s\n",GBLbuf.errmsg);
        exit(EXIT_FAILURE);
    }

    if (GBLbuf.isrOverrun) {
        (void)fprintf(stderr,
                      "%s: ISR overrun - base sampling rate is too fast\n",
                      QUOTE(MODEL));
        exit(EXIT_FAILURE);
    }

    if (ssGetErrorStatus(S) != NULL) {
        (void)fprintf(stderr,"%s\n", ssGetErrorStatus(S));
        exit(EXIT_FAILURE);
    }
#ifdef MULTITASKING
    else {
        int_T i;
        for (i=1; i<NUMST; i++) {
            if (GBLbuf.overrunFlags[i]) {
                (void)fprintf(stderr,
                        "%s ISR overrun - sampling rate is too fast for "
                        "sample time index %d\n", QUOTE(MODEL), i);
                exit(EXIT_FAILURE);
            }
        }
    }
#endif

    /* timing data */
    rt_DestroyTimingEngine(S);
#if NCSTATES > 0
    /* integration data */
    rt_DestroyIntegrationData(S);
#endif

    sfcnTerminate(S);
    return(EXIT_SUCCESS);

} /* end main */



/* EOF: grt_malloc_main.c */
