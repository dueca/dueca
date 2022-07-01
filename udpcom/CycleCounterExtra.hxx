/* ------------------------------------------------------------------   */
/*      item            : CycleCounterExtra.hxx
        made by         : Rene van Paassen
        date            : 200612
        category        : header file
        description     :
        changes         : 2001612 first version
        language        : C++
        copyright       : (c) 2020 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Get the count, removes the repeat nibble.

    A cycle count uses 28 bits of an unsigned int to code the cycle, and
    4 lowest-significant bits (nibble) to code a repeat.

    @returns       Cycle count.
 */
inline uint32_t cycleCount() const
{ return cycle_counter >> 4; }

/** Verify that the given cycle or integer follows the current
    counter cycle.

    @param cycle   Cycle to be tested
    @returns       True if the given to-be-tested cycle is one ahead over
                   the current one.
*/
inline bool cycleIsNext(uint32_t cycle) const
{ return (cycle_counter & ~0xf) + 0x10 == (cycle & ~0xf); }

/** Verify that this counter represents the previous cycle over the
    integer or cycle given. Handles wrap-around.

    @param cycle   Cycle to be tested
    @returns       True if the given to-be-tested cycle is one behind
                   the current one.
*/
inline bool cycleIsPrevious(uint32_t cycle) const
{ return (cycle_counter & ~0xf) == (cycle & ~0xf) + 0x10; }

/** Verify that this counter is equal to the given cycle, or at most two ahead.

    @param cycle   Cycle to be tested
    @returns       True if the given to-be-tested cycle is equal, one or
                   two behind to the current one.
*/
inline bool cycleIsCurrentOrPast(uint32_t cycle) const
{ return (cycle & ~0xf) == (cycle_counter & ~0xf) ||
    (cycle & ~0xf) + 0x10 == (cycle_counter & ~0xf) ||
    (cycle & ~0xf) + 0x20 == (cycle_counter & ~0xf); }

/** To check whether sufficient data has been received.

    The cycle counter for the data from each peer is compared to the
    cycle now given by the master. If the master now flags cycle "n",
    the peer cycles are sufficiently up to date if they all minimally
    at cycle n-1.

    The master may back-track by one cycle if the previous has not
    been correctly confirmed, so the master may also flag/repeat cycle
    n-1, while n has already been received by some nodes.

    @param cycle    Cycle from the master, as reference
    @returns        true, if the peer's cycle is one lower than the master's
                    (normal case), equal to the master's (repeated data),
                    or even one higher than the master (when master
                    backtracked).
*/
inline bool cycleIsUpToDate(uint32_t cycle) const
{
  return
    (cycle & ~0xf) == (cycle_counter & ~0xf) + 0x10 || // master backtracked
    (cycle & ~0xf) == (cycle_counter & ~0xf) ||        // a repeated cycle
    (cycle & ~0xf) + 0x10 == (cycle_counter & ~0xf);   // normal progress
}

/** See if this cycle has been processed */
//bool cycleHasBeenProcessed(uint32_t cycle) const;

/** Count cycle as current */
inline bool cycleIsCurrent(uint32_t cycle) const
{ return (cycle & ~0xf) == (cycle_counter & ~0xf); }

/** Increment cycle counter.

    @param ninc    Increment step.
    @returns       An incremented counter, with the repeat nibble set to 0.
*/
inline CycleCounter cycleIncrement(unsigned ninc=1)
{ return CycleCounter((cycle_counter & ~0xf) + (ninc << 4)); }

/** Decrement for recovery.

    @returns       A decremented counter, with the repeat nibble already
                   set to 1.
 */
inline void cycleBack()
{ cycle_counter = (cycle_counter & ~0xf) - 0xf; }

/** Increment repeat counter, use a nibble.

    The repeat counter nibble is increased or rolls over.
*/
inline void cycleRepeatIncrement()
{ cycle_counter = (((cycle_counter & 0xf) + 1) & 0xf) |
    (cycle_counter & ~0xf); }

/** Conversion to unsigned int, if needed */
inline operator uint32_t () const
{ return cycle_counter; }
