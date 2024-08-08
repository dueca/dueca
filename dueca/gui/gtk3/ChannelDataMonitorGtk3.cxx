/* ------------------------------------------------------------------   */
/*      item            : ChannelDataMonitor.cxx
        made by         : Rene' van Paassen
        date            : 180518
        category        : body file
        description     :
        changes         : 180518 first version
        language        : C++
        copyright       : (c) 18 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ChannelDataMonitor_cxx
#include "ChannelDataMonitorGtk3.hxx"
#include <debug.h>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <iomanip>
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

ChannelDataMonitorGtk3::ChannelDataMonitorGtk3(ChannelOverviewGtk3  *master,
                                               const std::string& channelname,
                                               unsigned channelno,
                                               unsigned entryno,
                                               const std::string& gladefile) :
  ChannelDataMonitor(master, channelno, entryno),
  window(),
  store(NULL)
{
  static GladeCallbackTable cb_table[] = {
    { "close", "clicked",
      gtk_callback(&_ThisModule_::cbClose) },
    { "refresh_data", "clicked",
      gtk_callback(&_ThisModule_::cbRefreshData) },
    { "channel_data_view", "delete_event",
      gtk_callback(&_ThisModule_::cbDelete)},
    { NULL }
  };

  bool res = window.readGladeFile
    (gladefile.c_str(), "channel_datamonitor",
     reinterpret_cast<gpointer>(this), cb_table);
  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the data monitor GUI. Check
       DUECA installation and paths.
    */
    E_CNF("failed to open channel data view");
    return;
  }

  GtkWidget *channeltree = GTK_WIDGET(window["channel_data_view"]);
  store = gtk_tree_store_new
    (int(S_nfields),
     G_TYPE_STRING,   // member name / index
     G_TYPE_STRING,   // data
     G_TYPE_BOOLEAN   // is leaf
     );
  g_object_ref_sink(G_OBJECT(store));
  gtk_tree_view_set_model(GTK_TREE_VIEW(channeltree),
                          GTK_TREE_MODEL(store));
  GtkCellRenderer *rendererlabel = gtk_cell_renderer_text_new();
  GtkCellRenderer *rendererdata = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(rendererdata), "family", "Monospace", NULL);

  // name column
  GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes
    ("name", rendererlabel, "text", S_membername, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(channeltree), col);
  //g_object_unref(G_OBJECT(col));

  // data column
  GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes
    ("value", rendererdata, "text", S_memberdata, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(channeltree), col2);
  gtk_tree_view_column_set_expand(col2, TRUE);
  //g_object_unref(G_OBJECT(col2));

  std::stringstream title;
  title << channelname << "  #" << entryno;
  gtk_window_set_title
    (GTK_WINDOW(window["channel_datamonitor"]), title.str().c_str());

  // g_object_unref(G_OBJECT(renderer));
}


ChannelDataMonitorGtk3::~ChannelDataMonitorGtk3()
{
  gtk_tree_store_clear(store);
  window.hide();
  g_object_unref(G_OBJECT(store));
}

void ChannelDataMonitorGtk3::cbClose(GtkButton* button, gpointer gp)
{

  master->closeMonitor(channelno, entryno);
}

void ChannelDataMonitorGtk3::cbRefreshData(GtkButton* button, gpointer gp)
{
  master->refreshMonitor(channelno, entryno);
}

gboolean ChannelDataMonitorGtk3::cbDelete(GtkWidget *window, GdkEvent *event,
                                          gpointer user_data)
{
  master->closeMonitor(channelno, entryno);
  return TRUE;
}

inline const char* print_string(const JValue &value)
{
  static char tmp[22];
  if (value.GetStringLength() == 1) {
    const char * str = value.GetString();
    if (str[0] < 32 || str[0] > 126) {
      snprintf(tmp, sizeof(tmp), "  (0x%02X)", (unsigned(str[0]) & 0xff));
    }
    else {
      snprintf(tmp, sizeof(tmp), "%s (0x%02X)", str, (unsigned(str[0]) & 0xff));
    }
    return tmp;
  }
  return value.GetString();
}

void ChannelDataMonitorGtk3::
insertJsonValue(GtkTreeIter* itname, const JValue &value)
{
  static const char* json_fix[] = { "null", "false", "true" };

  switch (value.GetType()) {
  case json::kNullType:
  case json::kFalseType:
  case json::kTrueType:
    gtk_tree_store_set
      (store, itname,
       S_memberdata, json_fix[value.GetType()], S_isleaf, TRUE, -1);
    break;
  case json::kStringType:
    gtk_tree_store_set
      (store, itname,
       S_memberdata, print_string(value), S_isleaf, TRUE, -1);
    break;
  case json::kNumberType:
    if (value.IsInt()) {
      gtk_tree_store_set
        (store, itname,
         S_memberdata,
         boost::str(boost::format("%5d") % value.GetInt()).c_str(),
         S_isleaf, TRUE, -1);
    }
    else if (value.IsInt64()) {
      gtk_tree_store_set
        (store, itname,
         S_memberdata,
         boost::str(boost::format("%5d") % value.GetInt64()).c_str(),
         S_isleaf, TRUE, -1);
    }
    else if (value.IsUint64()) {
      gtk_tree_store_set
        (store, itname,
         S_memberdata,
         boost::str(boost::format("%5d") % value.GetUint64()).c_str(),
         S_isleaf, TRUE, -1);
    }
    else if (value.IsDouble()) {
      double v = value.GetDouble();
      if (abs(v) < 10000.0 && abs(v) >= 1.0)
        gtk_tree_store_set
          (store, itname,
           S_memberdata,
           boost::str(boost::format("%11.5f") % value.GetDouble()).c_str(),
           S_isleaf, TRUE, -1);
      else {
        gtk_tree_store_set
          (store, itname,
           S_memberdata,
           boost::str(boost::format("%15.5e") % value.GetDouble()).c_str(),
           S_isleaf, TRUE, -1);
      }
    }
    else {
      assert(0);
    }
    break;
  default:
    assert(0);
  }
  removeIterChildren(itname);
}


void ChannelDataMonitorGtk3::
insertJsonArray(GtkTreeIter* itparent, const JValue &doc)
{
  gboolean itvalid;
  GtkTreeIter itname;
  descendIter(itvalid, itname, itparent);
  int idx = 0;

  for (JValue::ConstValueIterator it = doc.Begin();
       it != doc.End(); ++it) {
    std::stringstream name; name << std::setw(3) << idx++;

    // find a new iter
    checkOrCreateIter(itvalid, itname, itparent, name.str().c_str());

    if (it->IsObject()) {

      insertJson(&itname, *it);
    }
    else if (it->IsArray()) {

      insertJsonArray(&itname, *it);
    }
    else {

      insertJsonValue(&itname, *it);
    }

    // move to next iter, if it exists
    toNextIter(itvalid, itname);
  }

  // remove siblings, if somehow the array has shrunk
  removeIterSiblings(itvalid, itname);
}

void ChannelDataMonitorGtk3::
descendIter(gboolean& itvalid, GtkTreeIter& itname,
            GtkTreeIter* itparent)
{
  // find the first child of this parent
  itvalid = gtk_tree_model_iter_children
    (GTK_TREE_MODEL(store), &itname, itparent);
  DEB("descending iter")
}

void ChannelDataMonitorGtk3::
checkOrCreateIter(gboolean& itvalid, GtkTreeIter& itname,
                  GtkTreeIter* itparent, const char* name)
{
  // try to re-use existing rows, itname should point to a to-be-reused row
  if (itvalid) {
    // no action
    DEB("re-using row for name " << name);
  }
  else {
    // insert a new row, with the current parent (or top)
    gtk_tree_store_append(store, &itname, itparent);
    DEB("new row for name " << name);
  }
  gtk_tree_store_set(store, &itname, 0, name, -1);
}

void ChannelDataMonitorGtk3::
removeIterChildren(GtkTreeIter* parent)
{
  GtkTreeIter itchld;
  gboolean have = gtk_tree_model_iter_children
    (GTK_TREE_MODEL(store), &itchld, parent);
  while (have == TRUE) {
    have = gtk_tree_store_remove(store, &itchld);
    DEB("removed a child");
  }
}

void ChannelDataMonitorGtk3::
removeIterSiblings(gboolean& have, GtkTreeIter& itname)
{
  while(have == TRUE) {
    have = gtk_tree_store_remove(store, &itname);
    DEB("removed a sibling");
  }
}

void ChannelDataMonitorGtk3::
toNextIter(gboolean& have, GtkTreeIter& itname)
{
  if (have) {
    have = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &itname);
    DEB("check for next sibling");
  }
}

void ChannelDataMonitorGtk3::
insertJson(GtkTreeIter *itparent, const JValue &doc)
{
  gboolean itvalid;
  GtkTreeIter itname;
  descendIter(itvalid, itname, itparent);

  for (JValue::ConstMemberIterator it = doc.MemberBegin();
       it != doc.MemberEnd(); ++it) {

    // prepare a new iter, at least re-affirm the name
    checkOrCreateIter(itvalid, itname, itparent, it->name.GetString());

    if (it->value.IsObject()) {

      // recursively insert values
      insertJson(&itname, it->value);
    }
    else if (it->value.IsArray()) {

      // repeat array insert
      insertJsonArray(&itname, it->value);
    }
    else {
      insertJsonValue(&itname, it->value);
    }

    // move to next iter, if it exists
    toNextIter(itvalid, itname);
  }

  // remove siblings, if somehow the object has shrunk
  removeIterSiblings(itvalid, itname);
}


void ChannelDataMonitorGtk3::refreshData(const ChannelMonitorResult& rdata)
{
  std::stringstream timespan;
  timespan << rdata.ts_actual.getValidityStart();
  if (rdata.ts_actual.getValiditySpan()) {
    timespan << ", " << rdata.ts_actual.getValidityStart();
  }
  gtk_label_set_text(GTK_LABEL(window["timespan_label"]),
                     timespan.str().c_str());

  //gtk_tree_store_clear(store);
  if (rdata.json.size()) {

    JDocument doc;
    doc.Parse<json::kParseNanAndInfFlag>(rdata.json.c_str());

    insertJson(NULL, doc);
  }
  DEB1(rdata.json);
}

void ChannelDataMonitorGtk3::close()
{ window.hide(); }

void ChannelDataMonitorGtk3::open()
{ window.show(); }

DUECA_NS_END
