/* ------------------------------------------------------------------   */
/*      item            : UniqueFile.cxx
        made by         : Rene' van Paassen
        date            : 070503
        category        : body file
        description     :
        changes         : 070503 first version
        language        : C++
        copyright       : (c) 2007 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define UniqueFile_cxx
#include "UniqueFile.hxx"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <debug.h>
#include <cstring>

DUECA_NS_START

UniqueFile::UniqueFile() :
  std::ofstream(),
  ordinal(0)
{
  name[0] = '\000';
}

const char* UniqueFile::fileName(const char* fmt)
{
  int res = snprintf(name, sizeof(name), fmt, ordinal);

  if (res >= int(sizeof(name)) || res < 1)
    throw(UniqueFileError("file name too short or too long"));

  return name;
}

void UniqueFile::open(const char* fmt, unsigned start_at,
                      ios_base::openmode __mode)
{
  // this is a pretty brute-force method
  ordinal = start_at;
  struct stat stat_buf;
  errno = 0;
  while (ordinal < 10000) {
    if (stat(fileName(fmt), &stat_buf) == -1) {
      if (errno == ENOENT) break;
      /* DUECA extra.

         Unforseen error in checking for file name. Check folder
         permissions? */
      W_XTR("Uniquefile found " << name << " with error "
            << strerror(errno));
    }
    ordinal++;
  }
  /*  for ( ; ordinal < 10000 &&
        !(stat(fileName(fmt), &stat_buf) == -1 && errno == ENOENT); ordinal++);
  */

  if (ordinal < 10000) {
    std::ofstream::open(name, __mode);

    if (bad())
      throw(UniqueFileError("open failed"));
  }
  else {
    throw(UniqueFileError("no free file name"));
  }
}

UniqueFileError::UniqueFileError(const UniqueFileError& e) :
  reason(e.reason)
{
  //
}

UniqueFileError::UniqueFileError(const char* reason) :
  reason(reason)
{
  //
}

UniqueFileError::~UniqueFileError() throw()
{
  //
}

DUECA_NS_END
