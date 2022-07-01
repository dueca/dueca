/* ------------------------------------------------------------------   */
/*      item            : ConfigBuffer.hxx
        made by         : Rene van Paassen
        date            : 170205
        category        : header file
        description     :
        changes         : 170205 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ConfigBuffer_hxx
#define ConfigBuffer_hxx

#include <sstream>
#include <dueca.h>
#include <dueca/MessageBuffer.hxx>

/** Configuration messages communication */
class ConfigBuffer
{
  /** Buffer for receiving data and decoding */
  char *buffer;

  /** Index reading decoding buffer */
  size_t cindex;

  /** Fill level */
  size_t filllevel;

  /** Current buffer size */
  size_t bufsize;

public:
  /** Constructor
      @param defsize   Default buffer size */
  ConfigBuffer(size_t defsize=1024);

  /** Destructor */
  ~ConfigBuffer();

  /** Copy in a number of bytes */
  void write(const char* bytes, size_t len);

  /** Copy in a messagebuffer */
  void write( MessageBuffer::ptr_type msgbuf);

  /** Return an AmorphStore with the current position and fill level */
  dueca::AmorphReStore getStore();

  /** Remember remainder. Level is the number of bytes that have been
      read out. */
  void saveForLater(unsigned level);
};



#endif
