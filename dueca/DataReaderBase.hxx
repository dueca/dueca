/* ------------------------------------------------------------------   */
/*      item            : DataReaderBase.hxx
        made by         : Rene van Paassen
        date            : 140106
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 141130 split off from DataReader.hxx
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataReaderBase_hxx
#define DataReaderBase_hxx

#include <dueca_ns.h>
#include <TimeSpec.hxx>
#include <ChannelReadToken.hxx>

DUECA_NS_START;


class ChannelReadToken;

/** Base class for the DataReader derived templates.

    This keeps common data for the DataReader, and accesses the data
    by means of a void pointer. */
class DataReaderBase
{
  /** Let access objects manipulate my private/protected parts */
  friend class DataReaderBaseAccess;

protected:
  /** Reference to the channel access token. */
  ChannelReadToken &token;

  /** Time span or point requested */
  DataTimeSpec t_request;

  /** Time specification as realised. */
  DataTimeSpec ts_data;

  /** Data origin */
  GlobalId     data_origin;

  /** First access flag */
  bool         firstaccess;

protected:
  /** Constructor */
  DataReaderBase(ChannelReadToken &token, const DataTimeSpec &t_request) :
    token(token), t_request(t_request), firstaccess(true) { }
};

/** Base class for DataReader access template objects.

    The DataReader class can access the data in the channel in a
    variety of ways. A template parameter in this class determines how
    the data in the channel is found; either joining data from all
    entries matching the data type, getting data that matches a
    specific time, etc.
 */
class DataReaderBaseAccess {
protected:
  /** Get access to the data valid at a specific time. Uses the token
      of the DataReaderBase class. The token returns the data valid at,
      or the latest data before the requested time, check the returned
      time spec for the result.
      \param b         Reader base class, for token access.
      \param t         Time for which access is requested.
      \param ts_data   Time span or time point of the returned data.
      \param magic     Datatype magic number, for verification.
  */
  inline const void * getAccess(DataReaderBase& b, TimeTickType t,
                                DataTimeSpec& ts_data, unsigned int magic)
  { return b.token.getAccess(t, ts_data, b.data_origin, magic); }

  /** Access the DataReaderBase request time */
  inline const DataTimeSpec &t_request(DataReaderBase& b) {return b.t_request;}
  /** Access the DataReaderBase data time */
  inline DataTimeSpec &ts_data(DataReaderBase& b) { return b.ts_data; }
  /** Access the DataReaderBase access boolean */
  inline bool &firstaccess(DataReaderBase& b) { return b.firstaccess; }
  /** Release the read access again */
  inline void releaseAccess(ChannelReadToken& token, const void* data_ptr)
  { if (data_ptr) token.releaseAccess(data_ptr); }
  /** Release the read access again, from a base */
  inline void releaseAccess(DataReaderBase& b, const void* data_ptr)
  { if (data_ptr) b.token.releaseAccess(data_ptr); }
  /** Release the read access, but keep the current data */
  inline void releaseAccessKeepData(ChannelReadToken& token,
                                    const void* data_ptr)
  { if (data_ptr) token.releaseAccessKeepData(data_ptr); }
  /** Select the first entry in a multi-entry read */
  inline void selectFirstEntry(DataReaderBase& b)
  { b.token.selectFirstEntry(); }
  /** Is there an entry currently */
  inline bool haveEntry(DataReaderBase& b)
  { return b.token.haveEntry(); }
  /** Pass to the next entry */
  inline void selectNextEntry(DataReaderBase& b)
  { b.token.selectNextEntry(); }
  /** Get the id of the channel */
  inline const GlobalId& getChannelId(DataReaderBase& b)
  { return b.token.getChannelId(); }
  /** Get the id of the accessing client */
  inline const GlobalId& getClientId(DataReaderBase& b)
  { return b.token.getClientId(); }
  /** Is reading sequential, or indexed-access */
  inline bool isSequential(DataReaderBase& b)
  { return b.token.isSequential(); }
};

DUECA_NS_END;

#endif
