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

#include "gtk/gtk.h"
#define ChannelDataMonitor_csxx
#include "ChannelDataMonitorGtk4.hxx"
#include <debug.h>
#include <boost/format.hpp>
#include <sstream>
#include <iomanip>
#define DEBPRINTLEVEL -2
#include <debprint.h>

namespace {

/** Single data entry in the channel (data member, nested member or array) */
struct _DDataEntry
{
  // parent, for the type system
  GObject parent;

  // optionally, expanded sub-parts (from object or array)
  GListStore *children;

  // level, for indentation
  guint level;

  // link to the data model
  const ChannelDataViewPair *data;
};

// Properties of the entry class
enum DataEntryProperty { D_VALUE = 1, D_NDATAEPROP };

// create a type for the DCO data pieces
G_DECLARE_FINAL_TYPE(DDataEntry, d_data_entry, D, DATA_ENTRY, GObject);
G_DEFINE_TYPE(DDataEntry, d_data_entry, G_TYPE_OBJECT);

// value is dynamic, need to bind through property
static void d_data_entry_set_property(GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec)
{
#if 0
  DDataEntry *self = D_DATA_ENTRY(object);

  switch ((DataEntryProperty)property_id) {
  case D_VALUE:
    assert(strlen(g_value_get_string(value)) == 0);
    // self->data->value.insert(0, g_value_get_string(value));
    break;

  default:
      /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
#endif
}

static void d_data_entry_get_property(GObject *object, guint property_id,
                                      GValue *value, GParamSpec *pspec)
{
  DDataEntry *self = D_DATA_ENTRY(object);

  switch ((DataEntryProperty)property_id) {
  case D_VALUE:
    g_value_set_string(value, self->data->value.c_str());
    break;

  default:
      /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static GParamSpec *object_properties[D_NDATAEPROP] = {
  NULL, g_param_spec_string("value", "Value", "String value", "",
                            (GParamFlags)(G_PARAM_READWRITE |
                                          G_PARAM_EXPLICIT_NOTIFY |
                                          G_PARAM_CONSTRUCT))
};

static void d_data_entry_class_init(DDataEntryClass *_klass)
{
  auto klass = G_OBJECT_CLASS(_klass);
  klass->set_property = d_data_entry_set_property;
  klass->get_property = d_data_entry_get_property;
  g_object_class_install_properties(klass, G_N_ELEMENTS(object_properties),
                                    object_properties);
}

static void d_data_entry_init(DDataEntry *self)
{
  //
}

static DDataEntry *d_data_entry_new(const ChannelDataViewPair *p,
                                    guint level = 0)
{
  auto res = D_DATA_ENTRY(g_object_new(d_data_entry_get_type(), NULL));
  res->data = p;
  res->level = level;
  return res;
}

/* called when expander activated, produces a sub-list to show under the
   expander */
static GListModel *add_data_element(gpointer _item, gpointer user_data)
{
  auto item = D_DATA_ENTRY(_item);
  if (item->data->children.size()) {
    auto lm = g_list_store_new(d_data_entry_get_type());
    for (auto &c : item->data->children) {
      auto child = d_data_entry_new(&c, item->level + 1U);
      g_list_store_append(lm, gpointer(child));
      g_object_unref(child);
    }
    return G_LIST_MODEL(lm);
  }
  return NULL;
}

} // anonymous namespace

DUECA_NS_START

ChannelDataMonitorGtk4::ChannelDataMonitorGtk4(ChannelOverviewGtk4 *master,
                                               unsigned channelno,
                                               unsigned entryno,
                                               const std::string &gladefile) :
  ChannelDataMonitor(master, channelno, entryno),
  window()
{
  static GladeCallbackTable cb_table[] = {
    { "close", "clicked", gtk_callback(&_ThisModule_::cbClose) },
    { "refresh_data", "clicked", gtk_callback(&_ThisModule_::cbRefreshData) },
    { "channel_datamonitor", "close-request",
      gtk_callback(&_ThisModule_::cbDelete) },
    { "fact_elemname", "setup", gtk_callback(&_ThisModule_::cbSetupName) },
    { "fact_elemname", "bind", gtk_callback((&_ThisModule_::cbBindName)) },
    { "fact_elemvalue", "setup", gtk_callback(&_ThisModule_::cbSetupValue) },
    { "fact_elemvalue", "bind", gtk_callback((&_ThisModule_::cbBindValue)) },
    { NULL }
  };

  bool res = window.readGladeFile(gladefile.c_str(), "channel_datamonitor",
                                  reinterpret_cast<gpointer>(this), cb_table);
  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the data monitor GUI. Check
       DUECA installation and paths.
    */
    E_CNF("failed to open channel data view");
    return;
  }

  auto channeltree = GTK_COLUMN_VIEW(window["channel_data_view"]);
  store = g_list_store_new(d_data_entry_get_type());
  auto model = gtk_tree_list_model_new(G_LIST_MODEL(store), FALSE, FALSE,
                                       add_data_element, NULL, NULL);
  auto selection = gtk_no_selection_new(G_LIST_MODEL(model));
  gtk_column_view_set_model(channeltree, GTK_SELECTION_MODEL(selection));

  // set the title on the window
  std::stringstream title;
  title << master->getChannelName(channelno) << "  #" << entryno;
  gtk_window_set_title(GTK_WINDOW(window["channel_datamonitor"]),
                       title.str().c_str());

  // g_object_unref(G_OBJECT(renderer));
}

ChannelDataMonitorGtk4::~ChannelDataMonitorGtk4() { window.hide(); }

void ChannelDataMonitorGtk4::cbClose(GtkButton *button, gpointer gp)
{
  master->closeMonitor(channelno, entryno);
}

void ChannelDataMonitorGtk4::cbRefreshData(GtkButton *button, gpointer gp)
{
  master->refreshMonitor(channelno, entryno);
}

gboolean ChannelDataMonitorGtk4::cbDelete(GtkWidget *window, gpointer user_data)
{
  master->closeMonitor(channelno, entryno);
  return TRUE;
}

inline const char *print_string(const JValue &value)
{
  static char tmp[9];
  if (value.GetStringLength() == 1) {
    const char *str = value.GetString();
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

void ChannelDataMonitorGtk4::insertJsonValue(std::string &field,
                                             const JValue &value)
{
  static const char *json_fix[] = { "null", "false", "true" };

  switch (value.GetType()) {
  case json::kNullType:
  case json::kFalseType:
  case json::kTrueType:
    field = json_fix[value.GetType()];
    break;
  case json::kStringType:
    field = print_string(value);
    break;
  case json::kNumberType:
    if (value.IsInt()) {
      field = boost::str(boost::format("%5d") % value.GetInt());
    }
    else if (value.IsInt64()) {
      field = boost::str(boost::format("%5d") % value.GetInt64());
    }
    else if (value.IsUint64()) {
      field = boost::str(boost::format("%5d") % value.GetUint64());
    }
    else if (value.IsDouble()) {
      double v = value.GetDouble();
      if (abs(v) < 10000.0 && abs(v) >= 1.0)
        field = boost::str(boost::format("%11.5f") % value.GetDouble());
      else {
        field = boost::str(boost::format("%15.5e") % value.GetDouble());
      }
    }
    else {
      assert(0);
    }
    break;
  default:
    assert(0);
  }
}

void ChannelDataMonitorGtk4::insertJsonArray(dvplist_t &dl, dvplist_it li,
                                             const JValue &doc)
{
  int idx = 0;

  for (JValue::ConstValueIterator it = doc.Begin(); it != doc.End(); ++it) {
    std::stringstream name;
    name << std::setw(3) << idx++;        // with increment of index

    if (li == dl.end()) {

      // extend the data list if needed
      dl.emplace_back(name.str());
      li = std::prev(dl.end());
    }
    else {

      // overwrite current label to be sure
      li->label = name.str();
    }

    if (it->IsObject()) {

      // recursively insert object
      li->value = "";
      insertJson(li->children, li->children.begin(), *it);
    }
    else if (it->IsArray()) {

      // recursively insert array
      li->value = "[]";
      insertJsonArray(li->children, li->children.begin(), *it);
    }
    else {

      // insert the array member value
      insertJsonValue(li->value, *it);
    }

    // next array element
    li++;
  }

  // this may have shrunk, for variable-sized arrays & maps
  if (li != dl.end()) {
    dl.erase(li, dl.end());
  }
}

void ChannelDataMonitorGtk4::insertJson(dvplist_t &dl, dvplist_it li,
                                        const JValue &doc)
{
  for (JValue::ConstMemberIterator it = doc.MemberBegin();
       it != doc.MemberEnd(); ++it) {

    if (li == dl.end()) {
      dl.emplace_back(it->name.GetString());
      li = std::prev(dl.end());
    }
    else {
      li->label = it->name.GetString();
    }

    // prepare a new object
    if (it->value.IsObject()) {

      li->value = "";
      // recursively insert values into children
      insertJson(li->children, li->children.begin(), it->value);
    }
    else if (it->value.IsArray()) {

      li->value = "[]";
      // insert index and value into children
      insertJsonArray(li->children, li->children.begin(), it->value);
    }
    else {
      // insert value into value string
      insertJsonValue(li->value, it->value);
    }

    // to next iterator for the dvplist
    li++;
  }

  // this may have shrunk, for variable-sized arrays & maps
  if (li != dl.end()) {
    dl.erase(li, dl.end());
  }
}

// copy over the extracted list to a store
static void refreshTree(const dvplist_t &dl, GListStore *store);

static void alignData(const ChannelDataViewPair &it, DDataEntry *dl)
{
  if (it.children.size() && dl->children) {

    // this branch already opened, recursively update the data in the
    // g_list_store.
    refreshTree(it.children, G_LIST_STORE(dl->children));
  }
  else if (it.children.size() && dl->children == NULL) {

    // branch not opened, keep it that way
    // dl->children = g_list_store_new(d_data_entry_get_type());
  }
  else if (it.children.size() == 0 && dl->children) {

    // children gone now? can that be?
    g_object_unref(dl->children);
    dl->children = NULL;
  }

  // force set the data and notify the change
  dl->data = &it;
  g_object_notify_by_pspec(G_OBJECT(dl), object_properties[D_VALUE]);
}

static void refreshTree(const dvplist_t &dl, GListStore *store)
{
  auto len = g_list_model_get_n_items(G_LIST_MODEL(store));
  auto dllen = dl.size();

  // shrink if applicable
  if (dllen < len) {
    g_list_store_splice(store, dllen, len - dllen, NULL, 0);
    len = dllen;
  }

  unsigned idx = 0;
  for (auto const &d : dl) {
    auto dl = idx < len
                ? D_DATA_ENTRY(g_list_model_get_item(G_LIST_MODEL(store), idx))
                : d_data_entry_new(&d);
    alignData(d, dl);
    if (idx >= len) {
      g_list_store_append(store, dl);
      g_object_unref(dl);
    }
    idx++;
  }
}

void ChannelDataMonitorGtk4::refreshData(const ChannelMonitorResult &rdata)
{
  std::stringstream timespan;
  timespan << rdata.ts_actual.getValidityStart();
  if (rdata.ts_actual.getValiditySpan()) {
    timespan << ", " << rdata.ts_actual.getValidityStart();
  }
  gtk_label_set_text(GTK_LABEL(window["timespan_label"]),
                     timespan.str().c_str());

  // gtk_tree_store_clear(store);
  if (rdata.json.size()) {

    JDocument doc;
    doc.Parse(rdata.json.c_str());

    insertJson(data, data.begin(), doc);

    // refresh through binding
    refreshTree(data, store);
  }
  DEB1(rdata.json);
}

void ChannelDataMonitorGtk4::close() { window.hide(); }

void ChannelDataMonitorGtk4::open() { window.show(); }

bool ChannelDataMonitorGtk4::isOpen() const
{
  return gtk_widget_get_visible(GTK_WIDGET(window["channel_datamonitor"]));
}

void ChannelDataMonitorGtk4::cbSetupName(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto label = gtk_label_new("");
  auto expander = GTK_TREE_EXPANDER(gtk_tree_expander_new());
  gtk_tree_expander_set_child(expander, label);
  // gtk_tree_expander_set_indent_for_depth(expander, TRUE);
  // gtk_tree_expander_set_indent_for_icon(expander, FALSE);
  gtk_widget_set_halign(GTK_WIDGET(expander), GTK_ALIGN_START);
  // gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_list_item_set_child(item, GTK_WIDGET(expander));
}

  /** Create widgets for a value column */
void ChannelDataMonitorGtk4::cbSetupValue(GtkSignalListItemFactory *fact,
                                          GtkListItem *item, gpointer user_data)
{
  auto label = gtk_label_new("");
  // gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
  gtk_list_item_set_child(item, label);
}

/** Bind name data */
void ChannelDataMonitorGtk4::cbBindName(GtkSignalListItemFactory *fact,
                                        GtkListItem *item, gpointer user_data)
{
  auto expander = gtk_list_item_get_child(item);
  auto row = gtk_list_item_get_item(item);
  auto dat = D_DATA_ENTRY(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto label = gtk_tree_expander_get_child(GTK_TREE_EXPANDER(expander));
  if (dat->data->children.size()) {
    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(expander),
                                   GTK_TREE_LIST_ROW(row));
  }
  gtk_label_set_label(GTK_LABEL(label), dat->data->label.c_str());
  gtk_widget_set_margin_start(GTK_WIDGET(expander), dat->level * 8);
}

void ChannelDataMonitorGtk4::cbBindValue(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto row = gtk_list_item_get_item(item);
  auto dat = D_DATA_ENTRY(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (dat->data->children.size() == 0) {
    gtk_label_set_label(GTK_LABEL(label), dat->data->value.c_str());
    g_object_bind_property(dat, "value", label, "label", G_BINDING_DEFAULT);
    DEB("Binding " << row << " current value " << dat->data->value);
  }
}

DUECA_NS_END
