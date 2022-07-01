/* ------------------------------------------------------------------   */
/*      item            : ModuleStateExtra.hxx
        made by         : Rene' van Paassen
        date            : 130928
        category        : additional header code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Constructor from a string. */
ModuleState(const std::string& s);

/** Obtain the enumerated type. */
inline Type get() const {return t;}

/** Assignment with type. */
inline ModuleState &operator = (const ModuleState::Type& o)
{ t = o; return *this;}

/** Test equality. */
inline bool operator == (const ModuleState::Type& o) const
{return t == o;}

/** Test inequality. */
inline bool operator != (const ModuleState::Type& o) const
{return !(t == o);}

/** Combine two states, into a new one. */
ModuleState operator && (const ModuleState& o) const;

/** Combine and possibly change this state. */
ModuleState& operator &= (const ModuleState& o);

/** Set to neutral/clear status. */
inline void neutral() {t = Neutral;}

/** Obtain the state as a string value. */
const char* getString() const;
