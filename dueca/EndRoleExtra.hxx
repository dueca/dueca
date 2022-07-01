/* ------------------------------------------------------------------   */
/*      item            : EndRoleExtra.hxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional header code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Obtain a character representation. */
  const char* const getString() const;

  /** Assignment with type */
  inline EndRole &operator = (const EndRole::Type& o)
  { t = o; return *this;}

  /** Combine two endroles into one. */
  friend EndRole operator | (const EndRole& e1, const EndRole& e2);

  /** Combine two endroles into one. */
  friend EndRole operator & (const EndRole& e1, const EndRole& e2);
