/* ------------------------------------------------------------------   */
/*      item            : ChannelDataMonitor.hxx
        made by         : Rene van Paassen
        date            : 180518
        category        : header file
        description     :
        changes         : 180518 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelDataMonitorGtk3_hxx
#define ChannelDataMonitorGtk3_hxx

#include "ChannelOverviewGtk3.hxx"
#include <ChannelDataMonitor.hxx>
#include "GtkGladeWindow.hxx"
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <dueca_ns.h>

DUECA_NS_START

namespace json = rapidjson;
typedef json::GenericDocument<json::UTF8<> > JDocument;
typedef json::GenericValue<json::UTF8<> > JValue;

/** Class handling a channel entry view window */
class ChannelDataMonitorGtk3:
  public ChannelDataMonitor
{
  /** Define for easy use in tables */
  typedef ChannelDataMonitorGtk3 _ThisModule_;

  /** gtk window */
  GtkGladeWindow        window;

  /** Tree store for the object with data and widgets */
  GtkTreeStore         *store;

  /** Enumeration values for the store */
  enum StoreFields {
    S_membername,
    S_memberdata,
    S_isleaf,
    S_nfields
  };

public:
  /** Constructor */
  ChannelDataMonitorGtk3(ChannelOverviewGtk3  *master,
                         const std::string& channelname,
                         unsigned channelno,
                         unsigned entryno,
                         const std::string& gladefile);

  /** Destructor */
  ~ChannelDataMonitorGtk3();

  /** New data from the handler */
  void refreshData(const ChannelMonitorResult& rdata);

  /** close the window */
  void close();

  /** open the window */
  void open();

private:
  /** close callback, on close button in bar */
  void cbClose(GtkButton* button, gpointer gp);
  /** refresh data */
  void cbRefreshData(GtkButton* button, gpointer gp);
  /** window delete selected */
  gboolean cbDelete(GtkWidget *window, GdkEvent *event, gpointer user_data);
  /** helper */
  void insertJsonValue(GtkTreeIter* itname, const json::Value &value);
  /** helper */
  void insertJsonArray(GtkTreeIter* itparent,
                       const json::Value &doc);
  /** helper */
  void insertJson(GtkTreeIter* itparent,
                  const json::Value &doc);
  /** helper */
  void toNextIter(gboolean& have, GtkTreeIter& itname);
  /** helper */
  void removeIterSiblings(gboolean& have, GtkTreeIter& itname);
  /** helper */
  void removeIterChildren(GtkTreeIter* parent);
  /** helper */
  void checkOrCreateIter(gboolean& itvalid, GtkTreeIter& itname,
                    GtkTreeIter* itparent, const char* name);
  /** helper */
  void descendIter(gboolean& itvalid, GtkTreeIter& itname,
              GtkTreeIter* itparent);
};

DUECA_NS_END

#endif
