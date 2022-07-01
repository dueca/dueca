/* ------------------------------------------------------------------   */
/*      item            : DDFFExceptions.hxx
        made by         : Rene van Paassen
        date            : 211215
        category        : header file
        api             : DUECA_API
        description     :
        changes         : 211215 first version
        language        : C++
        copyright       : (c) 2021 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DDFFExceptions_hxx
#define DDFFExceptions_hxx

#include <exception>
#include "ddff_ns.h"
#include <dueca/SimTime.hxx>
#include <inttypes.h>

DDFF_NS_START

/** Exception information */
class file_read_error: public std::exception
{
  /** Error string */
  char str[64];

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return str; }

  /** Constructor */
  file_read_error(unsigned long offset=0);
};

/** Exception information */
class block_crc_error: public std::exception
{
  /** Error string */
  char str[100];

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return str; }

  /** Constructor */
  block_crc_error(uint64_t offset, uint32_t size);
};

/** Exception information */
class buffer_too_small: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return "reserved buffer too small"; }
};

/** Exception information */
class file_logic_error: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return "logic error, cannot write file"; }
};

/** Exception information */
class file_inconsistent_bufsize: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "inconsistent buffer sizes for stream"; }
};

/** Exception information */
class file_readonly_no_write: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "file opened read-only, cannot write"; }
};


/** Exception information */
class duplicate_filestreamread: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "duplicate filestreamread id requested"; }
};

/** Exception information */
class file_wrong_streamid: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "wrong stream ID returned from file"; }
};

/** Exception information */
class file_exists: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "File already exists, not allowed to overwrite"; }
};

/** Exception information */
class entry_exists: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "An entry with this name already exists"; }
};

/** Exception information */
class entry_notfound: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "Cannot find an entry with this name"; }
};

/** Exception information */
class file_missing: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "File does not exist"; }
};

/** Exception information */
class file_already_opened: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "File was already opened"; }
};

/** Exception information */
class incorrect_init: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "Incorrect initialisation"; }
};

/** Exception information */
class ddff_file_format_error: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "Unexpected data found"; }
};

/** cannot properly recreate the sequence of buffers */
class buffer_read_sequence_error: public std::exception
{
  /** Error string */
  char str[140];

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return str; }

  /** Constructor */
  buffer_read_sequence_error(unsigned stream_id, unsigned buffer_num,
                             unsigned expected_buffer_num,
                             unsigned offset, unsigned expected_offset);
};

/** Synchronization with the replay file is off */
class replay_synchronization: public std::exception
{
  /** Error string */
  char str[256];

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return str; }

  /** Constructor */
  replay_synchronization(const char* entity, unsigned recid,
                         TimeTickType treq0,  TimeTickType treq1,
                         TimeTickType tres0,  TimeTickType tres1);
};

/** Exception information */
class data_recorder_configuration_error: public std::exception
{
  /** Error string */
  char str[128];

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return str; }

  /** Constructor */
  data_recorder_configuration_error(const char* detail);
};

/** Incorrect/not available replay tag in Segments */
class cannot_find_segment: public std::exception
{
  /** Error string */
  char str[128];

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return str; }

  /** Constructor */
  cannot_find_segment(const char* entity, unsigned idx);
};

class tag_information_not_matching_recorders: public std::exception
{
  /** Error string */
  char str[128];

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return str; }

  /** Constructor */
  tag_information_not_matching_recorders(const char* entity, unsigned idx);
};

DDFF_NS_END

#endif
