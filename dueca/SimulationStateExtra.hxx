/* ------------------------------------------------------------------   */
/*      item            : SimulationStateExtra.hxx
        made by         : Rene' van Paassen
        date            : 130929
        category        : header file
        description     :
        changes         : 990713 first version
                          130929 adapted
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Constructor from a string */
SimulationState(const std::string& s);

/** Obtain a character representation. */
const char* const getString() const;

/** Obtain the enumerated type. */
inline Type get() const {return t;}

/** Assignment with type */
inline SimulationState &operator = (const SimulationState::Type& o)
{ t = o; return *this;}

/** Test equality */
inline bool operator == (const SimulationState::Type& o) const
{return t == o;}

/** Test inequality */
inline bool operator != (const SimulationState::Type& o) const
{return t != o;}

/** Combine two states, into a new one. */
SimulationState operator && (const SimulationState& o) const;

/** Combine and possibly change this state. */
SimulationState& operator &= (const SimulationState& o);

/** Returns true, if the state given in the argument is the same
    state or a "better" state (for example a final state, a compatible
    state) as the current one.
    \param o        Other state for comparison. */
bool betterOrSame(const SimulationState& o) const;

/** Return the next state after a transitional state, so with
    transition complete. */
Type transitionFinal() const;

/** Set to neutral/clear status. */
inline void neutral() {t = Neutral;}
