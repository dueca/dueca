/* ------------------------------------------------------------------   */
/*      item            : FindFiles.hxx
        made by         : Rene van Paassen
        date            : 070514
        category        : header file
        description     : This is a helper class to create a list of
                          file names that match a specific glob
                          pattern. Note that the files are not opened,
                          you simply get a list of names.
        changes         : 070514 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef FindFiles_hxx
#define FindFiles_hxx

#include <exception>
#include <vector>
#include <string>
#include <dueca_ns.h>


DUECA_NS_START

/** Exception to be thrown by FindFiles when an error occurs. */
class FindFilesError: public std::exception
{
  /** String explaining the problem. */
  const char* reason;

public:
  /** Copy constructor.
      \param   e         Original exception. */
  FindFilesError(const FindFilesError& e);

  /** Normal constructor.
      \param   reason    String explaining the why. */
  FindFilesError(const char* reason = "FindFiles error");

  /** Destructor. */
  virtual ~FindFilesError();

  /** Override the "what" of the standard class */
  const char* what() const throw() {return reason;}
};

/** Find a list of files matching a "glob" pattern. For example,

    \code
    FindFiles incos("*.inco");
    \endcode

    Will produce an object incos which lists all files with an
    extension .inco. Note that the files are not opened, you only get
    a list of names.

    The FindFiles object derives from the vector<string> template from
    the standard template library. You can use the normal methods to
    read out this object, e.g.
    \code
    for (FindFiles::const_iterator ii = incos.begin();
         ii != incos.end(); ii++) {
      // do something with *ii
    }

    // or
    for (int ii = 0; ii < incos.size(); ii++) {
      cout << incos[ii] << endl;
    }
    \endcode
*/
class FindFiles: public std::vector<std::string>
{
public:
  /** Default constructor, no arguments */
  FindFiles();

  /** Constructor with a pattern
      \param  pattern       glob pattern */
  FindFiles(const char* pattern);

  /** Re-scan a glob pattern. This discards old stuff!
      \param pattern        glob pattern */
  void scan(const char* pattern);
};



DUECA_NS_END

#endif
