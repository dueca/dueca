/* ------------------------------------------------------------------   */
/*      item            : FindFiles.cxx
        made by         : Rene' van Paassen
        date            : 070503
        category        : body file
        description     :
        changes         : 070503 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define FindFiles_cxx
#include "FindFiles.hxx"

#include <glob.h>
#include <iostream>

DUECA_NS_START

FindFiles::FindFiles()
{
  //
}

FindFiles::FindFiles(const char* pattern)
{
  try {
    scan(pattern);
  }
  catch (std::exception& e) {
    std::cerr << "In constructor " << e.what() << std::endl
              << "Object will be empty" << std::endl;
  }
}

void FindFiles::scan(const char* pattern)
{
  this->clear();
  glob_t globbuf;

  int err = glob(pattern, 0, NULL, &globbuf);
  switch(err) {
  case GLOB_NOSPACE:
    globfree(&globbuf);
    throw (FindFilesError("FindFiles: glob ran out of memory"));
#if defined(GLOB_ABORTED)
  case GLOB_ABORTED:
    globfree(&globbuf);
    throw (FindFilesError("FindFiles: glob read error"));
#endif
  case GLOB_NOMATCH:
  default:
    // it is no shame if there were no matches
    break;
  }
  for (size_t ii = 0; ii < globbuf.gl_pathc; ii++) {
    push_back(std::string(globbuf.gl_pathv[ii]));
  }
  globfree(&globbuf);
}


FindFilesError::FindFilesError(const FindFilesError& e) :
  reason(e.reason)
{
  //
}

FindFilesError::FindFilesError(const char* reason) :
  reason(reason)
{
  //
}

FindFilesError::~FindFilesError()
{
  //
}

DUECA_NS_END
