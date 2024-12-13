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

// Finally some useful hints:
// https://stackoverflow.com/questions/76711050/how-to-create-a-treeview-like-ui-using-columnview-in-gtk4

#pragma once

#include "ChannelOverviewGtk4.hxx"
#include <ChannelDataMonitor.hxx>
#include "GtkGladeWindow.hxx"
#include "gtk/gtk.h"
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <dueca_ns.h>

DUECA_NS_START

namespace json = rapidjson;
typedef json::GenericDocument<json::UTF8<>> JDocument;
typedef json::GenericValue<json::UTF8<>> JValue;

struct ChannelDataViewPair
{
  std::string label;
  std::string value;
  std::list<ChannelDataViewPair> children;
  ChannelDataViewPair(const std::string& label) :
    label(label) {}
};

typedef std::list<ChannelDataViewPair> dvplist_t;
typedef std::list<ChannelDataViewPair>::iterator dvplist_it;

// Gobject type system
struct _DDataEntry
{
  GObject parent;
  ChannelDataViewPair data;
};

/** Class handling a channel entry view window */
class ChannelDataMonitorGtk4 : public ChannelDataMonitor
{
  /** Define for easy use in tables */
  typedef ChannelDataMonitorGtk4 _ThisModule_;

  /** gtk window */
  GtkGladeWindow window;

  /** Tree store for the object with data and widgets */
  dvplist_t data;

public:
  /** Constructor */
  ChannelDataMonitorGtk4(ChannelOverviewGtk4 *master,
                          unsigned channelno,
                         unsigned entryno, const std::string &gladefile);

  /** Destructor */
  ~ChannelDataMonitorGtk4();

  /** New data from the handler */
  void refreshData(const ChannelMonitorResult &rdata) override;

  /** close the window */
  void close() override;

  /** open the window */
  void open() override;

  /** Current status */
  bool isOpen() const override;

private:
  /** Create widgets for a name columm */
  void cbSetupName(GtkSignalListItemFactory *fact, GtkListItem *object,
                   gpointer user_data);

  /** Create widgets for a value column */
  void cbSetupValue(GtkSignalListItemFactory *fact, GtkListItem *object,
                    gpointer user_data);

  /** Bind name data */
  void cbBindName(GtkSignalListItemFactory *fact, GtkListItem *object,
                  gpointer user_data);

  /** Bind value data */
  void cbBindValue(GtkSignalListItemFactory *fact, GtkListItem *object,
                   gpointer user_data);

  /** close callback, on close button in bar */
  void cbClose(GtkButton *button, gpointer gp);
  /** refresh data */
  void cbRefreshData(GtkButton *button, gpointer gp);
  /** window delete selected */
  gboolean cbDelete(GtkWidget *window, gpointer user_data);
  /** helper */
  void insertJsonValue(std::string &field, const json::Value &value);
  /** helper */
  void insertJsonArray(dvplist_t& dl, dvplist_it it, const json::Value &doc);
  /** helper */
  void insertJson(dvplist_t& dl, dvplist_it it, const json::Value &doc);
  /** helper */
  void removeIterSiblings(dvplist_t& dl, dvplist_it it);
  /** helper */
  void removeIterChildren(dvplist_t& dl, dvplist_it it);
};

DUECA_NS_END
