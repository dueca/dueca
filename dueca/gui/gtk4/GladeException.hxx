/* ------------------------------------------------------------------   */
/*      item            : GladeException.hxx
        made by         : Joost Ellerbroek
        date            : 100629
        category        : header file
        description     :
        changes         : 100629 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#ifndef GladeException_hxx
#define GladeException_hxx
#include <dueca_ns.h>
#include <exception>
#include <iostream>
#include <string>

DUECA_NS_START;

class GladeException : public std::exception
{
private:
  /** Why the exception was thrown. */
  std::string reason;

public:
  /** Copy constructor. */
  GladeException(const GladeException &e);

  /** Constructor */
  GladeException(const std::string& reason = "");

  /** Destructor. */
  virtual ~GladeException() throw();

  /** Return the reason for the exception. */
  inline const char* getReason() const {return reason.c_str();}

  /** Print to a stream. */
  std::ostream& print(std::ostream& os) const;
};

DUECA_NS_END;

#endif
