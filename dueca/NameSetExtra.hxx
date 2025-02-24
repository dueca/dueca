/* ------------------------------------------------------------------   */
/*      item            : NameSetExtra.hxx
        made by         : Rene' van Paassen
        date            : 130928
        category        : addition dco object
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Old compatibility constructor for a complete name, using a
    three-part structure, results in name c://e/p or c://e if p is an
    empty string.
    @param e    %Entity part
    @param c    %Class part
    @param p    %part
*/
NameSet(const std::string &e, const std::string &c, const std::string &p);

/** Old compatibility constructor for a complete name, with the part
    as an integer. Often used for automatically generating a series
    of names, results in name c://e/p.
    @param e    %Entity part
    @param c    %Class part
    @param p    %part, converted to string from integer
*/
NameSet(const std::string &e, const std::string &c, int p);

/** Return the entity part only */
std::string getEntity() const;

/** Return class name part only. */
std::string getClass() const;

/** Return part name(s) only. */
std::string getPart() const;

/** A nameset is considered smaller, when its name is alphabetically
    smaller.

    @param o The other nameset
    @returns true if smaller
*/
inline bool operator<(const NameSet &o) const { return this->name < o.name; }

/** A nameset is considered larger, when its name is alphabetically
    larger.

    @param o The other nameset
    @returns true if larger
*/
inline bool operator>(const NameSet &o) const { return this->name > o.name; }

/** validate, and throw an dueca::improper_nameset exception if required. */
void validate_set();
}
;

/** Exception class indicating that a string cannot be decoded as a nameset */
class improper_nameset : public std::exception
{
  /** To compose a proper message */
  std::stringstream message;

public:
  /** Constructor
      @param ns  failed string
    */
  improper_nameset(const std::string &ns);

  /** Re-implementation of std:exception what. */
  const char *what() const throw();
};

namespace {