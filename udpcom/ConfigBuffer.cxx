/* ------------------------------------------------------------------   */
/*      item            : ConfigBuffer.cxx
        made by         : Rene' van Paassen
        date            : 170205
        category        : body file
        description     :
        changes         : 170205 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ConfigBuffer_cxx
#include "ConfigBuffer.hxx"
#include <debug.h>

#define DEBPRINTLEVEL -1
#include <debprint.h>

ConfigBuffer::ConfigBuffer(size_t defsize) :
  buffer(new char[defsize]),
  cindex(0),
  filllevel(0),
  bufsize(defsize)
{

}


ConfigBuffer::~ConfigBuffer()
{

}

void ConfigBuffer::write(const char* bytes, size_t len)
{
  if (!len) return;

  DEB("extending config buffer from " << filllevel << " with " << len);
  if (len + filllevel > bufsize) {
    if (cindex > 0 && (len + filllevel <= bufsize + cindex)) {

      // can simply move down data and later fill
      for (size_t idx = 0; idx < filllevel - cindex; idx++) {
        buffer[idx] = buffer[idx + cindex];
      }
      filllevel -= cindex; cindex = 0;
    }
    else {

      // need to get a bigger buffer
      bufsize = bufsize * (2 + (len-1) / bufsize);
      char* newbuf = new char[bufsize];

      if (filllevel) {
        // copy the old stuff, and join in the new
        std::copy(&buffer[cindex], &buffer[filllevel], newbuf);
        filllevel -= cindex; cindex = 0;
      }

      // now move over the buffer
      delete [] buffer;
      buffer = newbuf;
    }
  }

  std::copy(bytes, &bytes[len], &buffer[filllevel]);
  filllevel += len;

  // copy in new data
  DEB("buffer accepted " << len << " bytes, now " <<
      cindex << " to " << filllevel);
}

void ConfigBuffer::write(MessageBuffer::ptr_type msgbuf)
{
  write(msgbuf->buffer, msgbuf->fill);
}

dueca::AmorphReStore ConfigBuffer::getStore()
{
  DEB("getting restore, from " << cindex << " to " << filllevel);
  return AmorphReStore(&buffer[cindex], filllevel-cindex);
}

void ConfigBuffer::saveForLater(unsigned level)
{
  cindex += level;
  if (cindex == filllevel) {
    cindex = 0; filllevel = 0;
    DEB("config buffer cleared");
  }
  else {
    DEB("remainder of buffer " << cindex << " to " << filllevel);
  }
}


