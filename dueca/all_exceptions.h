/* ------------------------------------------------------------------   */
/*      item            : all_exceptions.h
        made by         : Rene' van Paassen
        date            : 20010308
        category        : header file
        description     : Defines all the exceptions that DUECA or
                          DUSIME may throw at you
        language        : C++
        documentation   : DUECA_API
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// AccessToken
/** For some reason it is impossible to invalidate an access token. */
MAKE_EXCEPT(CannotInvalidateToken) ;

/** The channel end is not present. */
MAKE_EXCEPT(MissingChannelEnd) ;

/** The AmorphStore is full. */
MAKE_EXCEPT(AmorphStoreBoundary) ;

/** The AmorphReStore is empty. */
MAKE_EXCEPT(AmorphReStoreEmpty) ;

MAKE_EXCEPT(AmorphConversionUndefined);

MAKE_EXCEPT(MarkRange);

/** There is probably a pack/unpack mismatch, or data corruption. */
MAKE_EXCEPT(AmorphDataCorrupt) ;

/** An object could not be packed completely, but accepts partial
    transmission. */
MAKE_EXCEPT(IncompletePack) ;

// Channel
MAKE_EXCEPT(DoubleAccessRequested) ;
MAKE_EXCEPT(NoDataAvailable) ;
MAKE_EXCEPT(EntryNotAvailable) ;
MAKE_EXCEPT(AccessNotReleased) ;
MAKE_EXCEPT(AccessNotGranted) ;
/* MAKE_EXCEPT(AccessMisMatch) ; NOT USED */
MAKE_EXCEPT(NoEventsAvailable) ;
MAKE_EXCEPT(EventNotAvailable) ;
MAKE_EXCEPT(InvalidToken);
/** Using wrong data type in accessing the channel data */
MAKE_EXCEPT(ChannelWrongDataType);
// ChannelOrganiser
MAKE_EXCEPT(ChannelTypeConflict);
MAKE_EXCEPT(ChannelDistributionClash) ;

// Environment
MAKE_EXCEPT(NoLocalOutlet) ;
/* MAKE_EXCEPT(UnkownChannel) ; NOT USED AND MISSPELLED */
/* MAKE_EXCEPT(ChannelAlreadyExists) ; NOT USED */
/* MAKE_EXCEPT(StreamChannelAccessProblem) ; NOT USED */
MAKE_EXCEPT(InvalidChannelAccessReturn) ;
/* MAKE_EXCEPT(UnknownObjectType) ; NOT USED */

// Registry
MAKE_EXCEPT(ObjectAlreadyInRegistry) ;
/* MAKE_EXCEPT(RegistryIndexError) ; NOT USED */
/* MAKE_EXCEPT(RegistryNotLocked) ; */

// test purposes
/* MAKE_EXCEPT(TestException) ; NOT USED */

// Ticker
/* MAKE_EXCEPT(InvalidTickRequested) ; NOT USED */
/* MAKE_EXCEPT(ActivityAlreadyInTicker) ; NOT USED */
/* MAKE_EXCEPT(ActivityNotInTicker) ; NOT USED */

// Conditions
/* MAKE_EXCEPT(AttemptDoubleUseCondition); NOT USED */

// EntityManager, queryable stuff in general
/* MAKE_EXCEPT(DataQueryInterrupted); NOT USED */

// Event, for ownership of data part
MAKE_EXCEPT(CannotTransferOwnership);

// Activity views
MAKE_EXCEPT(WeaverKeyInvalid);

MAKE_EXCEPT(NoMemAvailable);

// modules throw this
MAKE_EXCEPT(CannotHandleState);

// configuration errors
MAKE_EXCEPT(NotAvailable);

// a trick
MAKE_EXCEPT(NeverThrown);
