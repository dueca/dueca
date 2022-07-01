/* ------------------------------------------------------------------   */
/*      item            : LogTimeExtra.hxx
        made by         : Rene van Paassen
        date            : 061205
        category        : header file
        description     : additional methods LogTime .dco
        changes         : 061205 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

  /** Compare to another log time.
      \param o    Other time
      \returns    true if this time is later. */
  bool operator > (const LogTime& o) const;

  /** Get the current time from the wall clock. */
  static LogTime now();

  /** Print the time, in full format; date + time
      \param os   Stream object to print to. */
  void show(std::ostream& os) const;

  /** Print the time, in short format; only time
      \param os   Stream object to print to. */
  void showtime(std::ostream& os) const;
