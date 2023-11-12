/* ------------------------------------------------------------------   */
/*      item            : DataTimeSpecExtra.hxx
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

/** Copy constructor, DataTimeSpec objects are often created from
      a PeriodicTimeSpec or TimeSpec object. */
  DataTimeSpec(const TimeSpec& time_spec);

  /** Constructor for a TimeSpec that starts and ends at the same
      time. For example events are like this. */
  DataTimeSpec(TimeTickType validity_start);

  /** Static function returning a DataTimeSpec with the current time */
  static DataTimeSpec now();

  /** Returns the time at which the interval starts */
  inline TimeTickType getValidityStart() const
  { return validity_start; }

  /** Returns the time at which the interval has ended */
  inline TimeTickType getValidityEnd() const
  { return validity_end; }

  /** Returns the size of the interval */
  inline TimeTickType getValiditySpan() const
  { return validity_end - validity_start; }

  /** Set the DataTimeSpec to the value of a normal TimeSpec */
  DataTimeSpec& operator = (const TimeSpec& other);

  /** Get the value of the interval in seconds */
  double getDtInSeconds() const;

  /** Find out how many microseconds elapsed since the formal start of
      this time. */
  int getUsecsElapsed() const;

  /** Allow my colleagues TimeSpec and PeriodicTimeSpec some leeway? */
  friend class TimeSpec;
  friend class PeriodicTimeSpec;

  /** Test whether this time spec is "after" the given time. */
  inline bool extendsOrIsAfter(const TimeTickType ts) const
  { return this->getValidityEnd() > ts; }

  /** Modify a time spec to not start earlier than ts */
  inline DataTimeSpec modifyToAfter(const TimeTickType ts) const
  { return DataTimeSpec(std::max(ts, this->getValidityStart()),
                        this->getValidityEnd()); }

  /** Add a certain value to the time */
  inline DataTimeSpec operator+ (const int& delta) const
  { return DataTimeSpec(validity_start + delta, validity_end + delta); }

  /** Move an interval up with a time in seconds */
  DataTimeSpec operator+ (const double delta) const;

  /** Move an interval up with a time in seconds */
  inline DataTimeSpec operator+ (const float delta) const
  { return *this + double(delta); }

  /** Subtract a certain value from the time */
  inline DataTimeSpec operator- (const int& delta) const
  { return DataTimeSpec(validity_start - delta, validity_end - delta); }

  /** Move an interval up with a time in seconds */
  DataTimeSpec operator- (const double delta) const;

  /** Move an interval up with a time in seconds */
  inline DataTimeSpec operator- (const float delta) const
  { return *this - double(delta); }

  /** Add a certain tick value to the time */
  inline DataTimeSpec& operator+= (const unsigned delta)
  { validity_start += delta; validity_end += delta; return *this; }

  /** Subtract a certain tick value from the time */
  inline DataTimeSpec& operator-= (const unsigned delta)
  { validity_start -= delta; validity_end -= delta; return *this;}

  /** Add a certain value to the time */
  inline DataTimeSpec& operator+= (const int delta)
  {
    if (delta >= 0)
      return this->operator+=(unsigned(delta));
    return this->operator-=(unsigned(-delta));
  }

  /** set the data span to zero */
  inline void setSpanToZero()
  { validity_end = validity_start; }

  /** Subtract a certain value from the time */
  inline DataTimeSpec& operator-= (const int delta)
  {
    if (delta >= 0)
      return this->operator-=(unsigned(delta));
    return this->operator+=(unsigned(-delta));
  }



