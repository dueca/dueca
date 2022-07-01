/* ------------------------------------------------------------------   */
/*      item            : IncoVariableExtra.hxx
        made by         : Rene' van Paassen
        date            : 130104
        category        : additional header code
        description     :
        changes         : 130102 first version
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/
  /// Constructor, for integer version
  IncoVariable(const char* name,
               int min_val, int max_val);

  /** Returns true if the variable is an integer. */
  inline bool isInteger() const { return vartype == IncoInt;}

  /** This can be used to specify the role that this inco variable has
      in a certain trim mode. It returns a pointer to itself, making a
      chained invocation possible, e.g.

      (new my_incovar("myvar", 0.0, 1.0, 0.001))
      ->forMode(some_mode, Constraint)
      ->forMode(some_other_mode, Control) */
  IncoVariable* forMode(IncoMode mode, IncoRole r);

  /** If true, then the IncoVariable supplies a value which will have
      to be inserted (used) in this trim calculation mode. */
  bool queryInsertForThisMode(IncoMode mode) const;

  /** If true, then the user can specify a value for this variable. */
  bool isUserControllable(IncoMode mode) const;

  /** Finds the role of the variable in this mode. */
  IncoRole findRole(IncoMode mode) const;

  /** Returns the name. */
  inline const char* getName() { return name.c_str();}

  /** Get the minimum allowed value. */
  float getMin() const { return min_value;}

  /** Get the maximum allowed value. */
  float getMax() const { return max_value;}
  /// \}
