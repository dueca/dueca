/* ------------------------------------------------------------------   */
/*      item            : ChannelOverviewGtk4.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Mon May  7 15:45:06 2018
        category        : body file
        description     :
        changes         : Mon May  7 15:45:06 2018 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#define ChannelOverviewGtk4_cxx

// include the definition of the module class
#include "ChannelOverviewGtk4.hxx"

// include the debug writing header, by default, write warning and
// error messages
#include <debug.h>

// include additional files needed for your calculation here
#include "ChannelDataMonitorGtk4.hxx"
#include "GtkDuecaView.hxx"

#include <boost/lexical_cast.hpp>
#include <dueca/ObjectManager.hxx>
#define DEBPRINTLEVEL -2
#include <debprint.h>
#include <dueca/DuecaPath.hxx>

// the standard package for DUECA, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca/dueca.h>

DUECA_NS_START

// class/module name
const char *const ChannelOverviewGtk4::classname = "channel-view";

// Parameters to be inserted
const ParameterTable *ChannelOverviewGtk4::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {

    { "count-delay",
      new VarProbe<_ThisModule_, unsigned>(&_ThisModule_::delay_countcollect),
      "Delay to wait before collecting a requested count" },

    { "glade-file",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::gladefile),
      "Interface description (glade, gtkbuilder) for the channel view window" },

    { "glade-file-monitor",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::monitor_gladefile),
      "Interface description (glade, gtkbuilder) for the monitor windows" },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "A module that presents an overview of channel access" }
  };

  return parameter_table;
}

// data type to hold reference to either channel info,
// entry info or reader info
enum _ChannelInfoType { Unknown, Channel, Entry, Reader };

// should re-design, with separate ChannelInfo, EntryInfo and ReaderInfo types
// count only on the entry and reader,
// sublist only on the channel and entry
// type to be detected
struct _DChannelInfo
{
  GObject parent;
  unsigned channel;
  unsigned entry;
  unsigned reader;
  uint32_t count;
  bool monitor;
  _ChannelInfoType type;

  GListStore *sublist;
};

// GTK4
// create a type for the DCO data pieces
G_DECLARE_FINAL_TYPE(DChannelInfo, d_channel_info, D, CHANNEL_INFO, GObject);
G_DEFINE_TYPE(DChannelInfo, d_channel_info, G_TYPE_OBJECT);

// constructor
ChannelOverviewGtk4::ChannelOverviewGtk4(Entity *e, const char *part,
                                         const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  ChannelOverview(e, part, ps),

  // initialize the data you need in your simulation or process
  gladefile(DuecaPath::prepend("channel_overview-gtk4.ui")),
  monitor_gladefile(DuecaPath::prepend("channel_datamonitor-gtk4.ui")),
  window(),
  menuaction(NULL),
  expand_subtree(&_ThisModule_::cbExpandEntriesReaders, this)
{
  //
}

DChannelInfo *d_channel_info_new(unsigned channel)
{
  auto res = D_CHANNEL_INFO(g_object_new(d_channel_info_get_type(), NULL));
  res->channel = channel;
  res->type = Channel;
  return res;
}

DChannelInfo *d_channel_info_new(unsigned channel, unsigned entry)
{
  auto res = D_CHANNEL_INFO(g_object_new(d_channel_info_get_type(), NULL));
  res->channel = channel;
  res->entry = entry;
  res->type = Entry;
  return res;
}

DChannelInfo *d_channel_info_new(unsigned channel, unsigned entry,
                                 unsigned reader)
{
  auto res = D_CHANNEL_INFO(g_object_new(d_channel_info_get_type(), NULL));
  res->channel = channel;
  res->entry = entry;
  res->reader = reader;
  res->type = Reader;
  return res;
}

enum ChannelInfoProperty { D_CINFO_COUNT = 1, D_MONITOR, D_NPROPERTIES };

static void d_channel_info_set_property(GObject *object, guint property_id,
                                        const GValue *value, GParamSpec *pspec)
{
  DChannelInfo *self = D_CHANNEL_INFO(object);

  switch ((ChannelInfoProperty)property_id) {
  case D_CINFO_COUNT:
    self->count = g_value_get_uint(value);
    break;

  case D_MONITOR:
    self->monitor = g_value_get_boolean(value);
    break;

  default:
      /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void d_channel_info_get_property(GObject *object, guint property_id,
                                        GValue *value, GParamSpec *pspec)
{
  DChannelInfo *self = D_CHANNEL_INFO(object);

  switch ((ChannelInfoProperty)property_id) {
  case D_CINFO_COUNT:
    g_value_set_uint(value, self->count);
    break;

  case D_MONITOR:
    g_value_set_boolean(value, self->monitor);
    break;

  default:
      /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static GParamSpec *object_properties[D_NPROPERTIES] = {
  NULL,
  g_param_spec_uint("rwcount", "Count", "Counter", 0, 0xffffffff, 0,
                    G_PARAM_READWRITE),
  g_param_spec_boolean("monitor", "Monitor", "state of monitor window", FALSE,
                       G_PARAM_READWRITE)
};

void d_channel_info_set_count(DChannelInfo *obj, unsigned count)
{
  obj->count = count;
  g_object_notify_by_pspec(G_OBJECT(obj), object_properties[D_CINFO_COUNT]);
}

void d_channel_info_set_monitor(DChannelInfo *obj, bool monitor)
{
  obj->monitor = monitor;
  g_object_notify_by_pspec(G_OBJECT(obj), object_properties[D_MONITOR]);
}

static void d_channel_info_class_init(DChannelInfoClass *_klass)
{
  auto klass = G_OBJECT_CLASS(_klass);
  klass->set_property = d_channel_info_set_property;
  klass->get_property = d_channel_info_get_property;
  g_object_class_install_properties(klass, G_N_ELEMENTS(object_properties),
                                    object_properties);
}

static void d_channel_info_init(DChannelInfo *self) {}

void sublist_destroyed(gpointer _ci, GObject *oldlist)
{
  auto ci = D_CHANNEL_INFO(_ci);
  ci->sublist = NULL;
}

GListModel *ChannelOverviewGtk4::cbExpandEntriesReaders(gpointer _item,
                                                        gpointer user_data)
{
  auto item = D_CHANNEL_INFO(_item);

  // there should NOT be a list on this row now!
  assert(item->sublist == NULL);

  // if this row has a channel, and it has entries, return these in a list
  if (item->type == Channel && infolist[item->channel]) {
    auto lm = g_list_store_new(d_channel_info_get_type());
    for (unsigned ii = 0; ii < infolist[item->channel]->entries.size(); ii++) {
      if (infolist[item->channel]->entries[ii]) {
        g_list_store_append(lm, d_channel_info_new(item->channel, ii));
      }
    }

    // ensure the sublist is reset to zero when the list is removed again
    g_object_weak_ref(G_OBJECT(lm), sublist_destroyed, item);
    item->sublist = lm;
    DEB("expand for entries, channel " << item->channel << " list "
                                       << reinterpret_cast<void *>(lm)
                                       << " check " << G_IS_LIST_STORE(lm));

    return G_LIST_MODEL(lm);
  }

  // if this row has an entry, and that has readers, return in a list
  if (item->type == Entry && infolist[item->channel] &&
      infolist[item->channel]->entries[item->entry]) {
    auto lm = g_list_store_new(d_channel_info_get_type());
    for (auto &r : infolist[item->channel]->entries[item->entry]->rdata) {
      g_list_store_append(
        lm, gpointer(d_channel_info_new(item->channel, item->entry,
                                        r->rdata.creationid)));
    }
    item->sublist = lm;
    DEB("expand for readers, channel "
        << item->channel << " entry " << item->entry << " list "
        << reinterpret_cast<void *>(lm) << " check " << G_IS_LIST_STORE(lm));
    g_object_weak_ref(G_OBJECT(lm), sublist_destroyed, item);
    return G_LIST_MODEL(lm);
  }

  // leaf node, never anything to expand, null list.
  item->sublist = NULL;
  return G_LIST_MODEL(NULL);
}

static GdkPaintable *stream_icon = NULL;
static GdkPaintable *event_icon = NULL;
static GdkPaintable *sequent_icon[2] = { NULL, NULL };
static GdkPaintable *select_icon[3] = { NULL, NULL, NULL };

// namespace

inline GdkPaintable *loadTextureFromFile(const char *fname)
{
  GError *error = NULL;
  auto tex = gdk_texture_new_from_file(
    g_file_new_for_path(DuecaPath::prepend(fname).c_str()), &error);
  if (error) {
    /* DUECA UI.

       Cannot load icon texture. Check DUECA installation.
    */
    E_XTR("Could not load texture \"" << fname << "\": " << error->message);
    g_error_free(error);
    error = NULL;
    return NULL;
  }
  g_object_ref(G_OBJECT(tex));
  return GDK_PAINTABLE(tex);
}

gint smallerString(gconstpointer _1, gconstpointer _2, gpointer self)
{
  auto const &infolist =
    reinterpret_cast<ChannelOverview *>(self)->getInfoList();
  auto r1 = GTK_TREE_LIST_ROW(const_cast<gpointer>(_1));
  auto r2 = GTK_TREE_LIST_ROW(const_cast<gpointer>(_2));
  auto const c1 = D_CHANNEL_INFO(gtk_tree_list_row_get_item(r1));
  auto const c2 = D_CHANNEL_INFO(gtk_tree_list_row_get_item(r2));

  if (infolist[c1->channel]->name < infolist[c2->channel]->name)
    return -1;
  if (infolist[c1->channel]->name > infolist[c2->channel]->name)
    return 1;

  // channel nnames are equal, next comparison, channel with "anything else"
  if (c1->type == Channel) 
    return -1;
  if (c2->type == Channel)
    return 1;

  // now compare on entry no
  if (c1->entry < c2->entry)
    return -1;
  if (c1->entry > c2->entry)
    return 1;

  // channel and entry numbers are equal, next on reader
  if (c1->reader < c2->reader)
    return -1;
  if (c1->reader > c2->reader)
    return 1;

  return 0;
}

gint smallerNumber(gconstpointer _1, gconstpointer _2, gpointer self)
{
  auto r1 = GTK_TREE_LIST_ROW(const_cast<gpointer>(_1));
  auto r2 = GTK_TREE_LIST_ROW(const_cast<gpointer>(_2));
  auto const c1 = D_CHANNEL_INFO(gtk_tree_list_row_get_item(r1));
  auto const c2 = D_CHANNEL_INFO(gtk_tree_list_row_get_item(r2));
  if (c1->channel < c2->channel)
    return -1;
  if (c1->channel > c2->channel)
    return 1;
  
  // channel numbers are equal, next comparison, channel with "anything else"
  if (c1->type == Channel) 
    return -1;
  if (c2->type == Channel)
    return 1;

  // now compare on entry no
  if (c1->entry < c2->entry)
    return -1;
  if (c1->entry > c2->entry)
    return 1;

  // channel and entry numbers are equal, next on reader
  if (c1->reader < c2->reader)
    return -1;
  if (c1->reader > c2->reader)
    return 1;

  return 0;
}

bool ChannelOverviewGtk4::complete()
{
  static GladeCallbackTable cb_table[] = {

    // set of callbacks for the different buttons in the inteface
    { "close", "clicked", gtk_callback(&_ThisModule_::cbClose) },
    { "refresh_times", "clicked",
      gtk_callback(&_ThisModule_::cbRefreshCounts) },
    { "channel_view", "close-request", gtk_callback(&_ThisModule_::cbHide) },

    // for each of the columns in the column view, bind to one of my set-up
    // functions, which create the widgets to be shown there, and to
    // a bind function, which fills these widgets with data from the
    // application model.
    { "fact_channelnum", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "fact_channelnum", "bind",
      gtk_callback(&_ThisModule_::cbBindChannelNum) },
    { "fact_channelname", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "fact_channelname", "bind",
      gtk_callback(&_ThisModule_::cbBindChannelName) },
    { "fact_entrynum", "setup", gtk_callback(&_ThisModule_::cbSetupExpander) },
    { "fact_entrynum", "bind", gtk_callback(&_ThisModule_::cbBindEntryNum) },
    { "fact_writerid", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "fact_writerid", "bind", gtk_callback(&_ThisModule_::cbBindWriterId) },
    { "fact_es", "setup", gtk_callback(&_ThisModule_::cbSetupImage) },
    { "fact_es", "bind", gtk_callback(&_ThisModule_::cbBindES) },
    { "fact_nwrites", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "fact_nwrites", "bind", gtk_callback(&_ThisModule_::cbBindNWrites) },
    { "fact_readerid", "setup", gtk_callback(&_ThisModule_::cbSetupExpander) },
    { "fact_readerid", "bind", gtk_callback(&_ThisModule_::cbBindReaderId) },
    { "fact_nreads", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "fact_nreads", "bind", gtk_callback(&_ThisModule_::cbBindNReads) },
    { "fact_sel", "setup", gtk_callback(&_ThisModule_::cbSetupImage) },
    { "fact_sel", "bind", gtk_callback(&_ThisModule_::cbBindSel) },
    { "fact_seq", "setup", gtk_callback(&_ThisModule_::cbSetupImage) },
    { "fact_seq", "bind", gtk_callback(&_ThisModule_::cbBindSeq) },
    { "fact_view", "setup", gtk_callback(&_ThisModule_::cbSetupCheckbox) },
    { "fact_view", "bind", gtk_callback(&_ThisModule_::cbBindView) },

    { NULL }
  };

  bool res = ChannelOverview::complete();
  if (!res) {
    /* DUECA UI.

       Error in initialization of ChannelOverview base module.
    */
    E_CNF("failed to start base overview complete");
    return res;
  }

  res = window.readGladeFile(gladefile.c_str(), "channel_view",
                             reinterpret_cast<gpointer>(this), cb_table);
  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the channel view GUI. Check
       DUECA installation and paths.
    */
    E_CNF("failed to open channel overview " << gladefile);
    return res;
  }

  // remember window
  channel_window = window["channel_view"];

  // access the columnview from the ui file, and link with the data model
  channel_tree = GTK_COLUMN_VIEW(window["channel_col_view"]);
  // the backing is a gtk list store with a custom-defined g object, which
  // only links to the data in the application
  store = g_list_store_new(d_channel_info_get_type());

  // expand_subtree = gtk_callback(&_ThisModule_::cbExpandEntriesReaders, this);
  auto model =
    gtk_tree_list_model_new(G_LIST_MODEL(store), FALSE, FALSE,
                            expand_subtree.c_callback(), &expand_subtree, NULL);
  auto cvsorter = gtk_column_view_get_sorter(channel_tree);
  auto sort_model = gtk_sort_list_model_new(G_LIST_MODEL(model), cvsorter);
  auto selection = gtk_no_selection_new(G_LIST_MODEL(sort_model));
  gtk_column_view_set_model(channel_tree, GTK_SELECTION_MODEL(selection));

  // and set the sorters
  auto namesort = gtk_custom_sorter_new(smallerString, this, NULL);
  auto numsort = gtk_custom_sorter_new(smallerNumber, NULL, NULL);
  gtk_column_view_column_set_sorter(
    GTK_COLUMN_VIEW_COLUMN(window.getObject("channel_col_name")), GTK_SORTER(namesort));
  gtk_column_view_column_set_sorter(
    GTK_COLUMN_VIEW_COLUMN(window.getObject("channel_col_num")), GTK_SORTER(numsort));
  g_object_unref(namesort);
  g_object_unref(numsort);

  // create icons for different channel types
  if (stream_icon == NULL) {
    event_icon = loadTextureFromFile("pixmaps/event-logo.png");
    stream_icon = loadTextureFromFile("pixmaps/stream-logo.png");
    select_icon[0] = loadTextureFromFile("pixmaps/number-logo.png");
    select_icon[1] = loadTextureFromFile("pixmaps/label-logo.png");
    select_icon[2] = loadTextureFromFile("pixmaps/multi-logo.png");
    sequent_icon[0] = loadTextureFromFile("pixmaps/sequential-logo.png");
    sequent_icon[1] = loadTextureFromFile("pixmaps/picking-logo.png");
    if (!event_icon || !stream_icon || !select_icon[0] || !select_icon[1] ||
        !select_icon[2] || !sequent_icon[0] || !sequent_icon[1])
      return false;
  }

  menuaction = GtkDuecaView::single()->requestViewEntry(
    "channelview", "Channel View", G_OBJECT(channel_window));

  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
ChannelOverviewGtk4::~ChannelOverviewGtk4() {}

static bool findPlaceInList(GListModel *list, unsigned &loc,
                            const std::function<int(gpointer)> &compareIt)
{
  auto n = g_list_model_get_n_items(list);
  for (auto i = n; i--;) {
    gpointer item = g_list_model_get_item(list, i);
    switch (compareIt(item)) {
    case 0: // match
      loc = i;
      return true;
    case -1: // item in list is smaller
      loc = i + 1;
      return false;
    case 1: // item in list (still) bigger
      break;
    }
  }
  loc = 0;
  return false;
}

DChannelInfo *ChannelOverviewGtk4::findChannel(unsigned ichan, unsigned &idxc)
{
  if (findPlaceInList(G_LIST_MODEL(store), idxc, [ichan](gpointer i) {
        if (D_CHANNEL_INFO(i)->channel == ichan)
          return 0;
        if (D_CHANNEL_INFO(i)->channel > ichan)
          return 1;
        return -1;
      })) {
    return D_CHANNEL_INFO(g_list_model_get_item(G_LIST_MODEL(store), idxc));
  }
  else {
    idxc = 0;
    return NULL;
  }
}

DChannelInfo *ChannelOverviewGtk4::findEntry(unsigned ichan, unsigned ientry,
                                             unsigned &idxc, unsigned &idxe)
{
  auto chn = findChannel(ichan, idxc);
  if (chn && chn->sublist) {
    if (findPlaceInList(G_LIST_MODEL(chn->sublist), idxe, [ientry](gpointer i) {
          if (D_CHANNEL_INFO(i)->entry == ientry)
            return 0;
          if (D_CHANNEL_INFO(i)->entry > ientry)
            return 1;
          return -1;
        })) {
      return D_CHANNEL_INFO(
        g_list_model_get_item(G_LIST_MODEL(chn->sublist), idxe));
    }
  }
  return NULL;
}

void ChannelOverviewGtk4::reflectChanges(unsigned ichan)
{
  // channels (should have) consecutive numbering corresponding to the store?
  unsigned idx;
  if (findPlaceInList(G_LIST_MODEL(store), idx, [ichan](gpointer i) {
        if (D_CHANNEL_INFO(i)->channel == ichan)
          return 0;
        if (D_CHANNEL_INFO(i)->channel > ichan)
          return 1;
        return -1;
      })) {

    // channel was found, notify of the change
    g_list_model_items_changed(G_LIST_MODEL(store), idx, 0, 0);
  }
  else {

    // new channel to the list at place idx
    auto citem = reinterpret_cast<gpointer>(d_channel_info_new(ichan));
    g_list_store_splice(store, idx, 0, &citem, 1);
  }
}

// @todo: solve crash when activated row is deleted
void ChannelOverviewGtk4::reflectChanges(unsigned ichan, unsigned ientry)
{
  if (infolist[ichan]->entries[ientry]) {
    DEB("Channel" << ichan << " adding/modifying entry " << ientry);
  }
  else {
    DEB("Channel" << ichan << " removing entry " << ientry);
  }

  // channel must be there already
  unsigned idxc;
  if (!findPlaceInList(G_LIST_MODEL(store), idxc, [ichan](gpointer i) {
        if (D_CHANNEL_INFO(i)->channel == ichan)
          return 0;
        if (D_CHANNEL_INFO(i)->channel > ichan)
          return 1;
        return -1;
      })) {
    assert(0);
  }

  // find the channel info entry
  auto citem = D_CHANNEL_INFO(g_list_model_get_item(G_LIST_MODEL(store), idxc));

  // now find the entry in the sublist.
  if (citem->sublist) {

    DEB("Changing entry in channel "
        << citem->channel << " list "
        << reinterpret_cast<void *>(citem->sublist));
    unsigned idxe;
    if (findPlaceInList(
          G_LIST_MODEL(citem->sublist), idxe, [ientry](gpointer i) {
            DEB("Entry pointer " << i);
            auto ei = D_CHANNEL_INFO(i);
            DEB("Pointer to entry info " << reinterpret_cast<void *>(ei));
            if (ei->entry == ientry)
              return 0;
            if (ei->entry > ientry)
              return 1;
            return -1;
          })) {

      // entry may be changed or deleted?
      if (infolist[ichan]->entries[ientry].get()) {

        // changed
        g_list_model_items_changed(G_LIST_MODEL(citem->sublist), idxe, 0, 0);
      }
      else {

        // deleted
        g_list_store_splice(citem->sublist, idxe, 1, NULL, 0);
      }
    }
    else {

      // new entry at location idxe
      auto eitem =
        reinterpret_cast<gpointer>(d_channel_info_new(ichan, ientry));
      g_list_store_splice(citem->sublist, idxe, 0, &eitem, 1);
      g_object_unref(eitem);
    }
  }
  g_list_model_items_changed(G_LIST_MODEL(store), idxc, 0, 0);
}

void ChannelOverviewGtk4::reflectChanges(unsigned ichan, unsigned ientry,
                                         unsigned ireader)
{
#if DEBPRINTLEVEL >= 0

  if (infolist[ichan]->entries[ientry]->getReader(ireader) !=
      infolist[ichan]->entries[ientry]->rdata.end()) {
    DEB("Channel" << ichan << " e " << ientry << " adding/modifying reader "
                  << ireader);
  }
  else {
    DEB("Channel" << ichan << " e " << ientry << " removing reader "
                  << ireader);
  }
#endif

  // channel must be there already
  unsigned idxc;
  if (!findPlaceInList(G_LIST_MODEL(store), idxc, [ichan](gpointer i) {
        if (D_CHANNEL_INFO(i)->channel == ichan)
          return 0;
        if (D_CHANNEL_INFO(i)->channel > ichan)
          return 1;
        return -1;
      })) {
    assert(0);
  }

  // find the channel info entry
  auto citem = D_CHANNEL_INFO(g_list_model_get_item(G_LIST_MODEL(store), idxc));

  // now find the entry in the sublist.
  if (citem->sublist) {
    unsigned idxe;
    if (!findPlaceInList(G_LIST_MODEL(citem->sublist), idxe,
                         [ientry](gpointer i) {
                           if (D_CHANNEL_INFO(i)->entry == ientry)
                             return 0;
                           if (D_CHANNEL_INFO(i)->entry > ientry)
                             return 1;
                           return -1;
                         })) {
      assert(0);
    }

    // find the entry info
    auto eitem =
      D_CHANNEL_INFO(g_list_model_get_item(G_LIST_MODEL(citem->sublist), idxe));

    // re-splice the entry in, so that any changes are reflected
    // g_object_ref(eitem);
    // g_list_store_splice(citem->sublist, idxe, 1,
    //                    reinterpret_cast<gpointer *>(&eitem), 1);
    // g_object_unref(eitem);

    // is the reader list visible?
    if (eitem->sublist) {

      // find the reader info in the overview list
      auto itr = infolist[ichan]->entries[ientry]->getReader(ireader);

      // find the index in the reader list
      unsigned idxr;
      if (findPlaceInList(G_LIST_MODEL(eitem->sublist), idxr,
                          [ireader](gpointer i) {
                            if (D_CHANNEL_INFO(i)->reader == ireader)
                              return 0;
                            if (D_CHANNEL_INFO(i)->reader > ireader)
                              return 1;
                            return -1;
                          })) {

        if (itr == infolist[ichan]->entries[ientry]->rdata.end()) {

          // gone from the overview, remove also from the reader list
          g_list_store_splice(eitem->sublist, idxr, 1, NULL, 0);
        }
        else {

          // indicate change in the readers list
          g_list_model_items_changed(G_LIST_MODEL(eitem->sublist), idxr, 0, 0);
        }
      }
      else {

        // add to the list
        auto ritem = reinterpret_cast<gpointer>(
          d_channel_info_new(ichan, ientry, ireader));
        g_list_store_splice(eitem->sublist, idxr, 0, &ritem, 1);
        g_object_unref(ritem);
      }
    }

    // indicate change in the entries list
    g_list_model_items_changed(G_LIST_MODEL(citem->sublist), idxe, 0, 0);
  }

  // indicate chagne in the channel list
  g_list_model_items_changed(G_LIST_MODEL(store), idxc, 0, 0);
}

inline ChannelOverview::ChannelInfoSet *
findInfo(const ChannelOverview::infolist_t &info, unsigned channel)
{
  if (channel >= info.size())
    return NULL;
  return info[channel].get();
}

inline ChannelOverview::ChannelInfoSet::EntryInfoSet *
findInfo(const ChannelOverview::infolist_t &info, unsigned channel,
         unsigned entry)
{
  auto ch = findInfo(info, channel);
  if (!ch)
    return NULL;
  if (entry >= ch->entries.size())
    return NULL;
  return ch->entries[entry].get();
}

inline ChannelOverview::ChannelInfoSet::EntryInfoSet::ReadInfoSet *
findInfo(const ChannelOverview::infolist_t &info, unsigned channel,
         unsigned entry, unsigned reader)
{
  auto en = findInfo(info, channel, entry);
  if (!en)
    return NULL;
  auto rdr = en->getReader(reader);
  if (rdr != en->rdata.end()) {
    return (*rdr).get();
  }
  return NULL;
}

void ChannelOverviewGtk4::reflectCounts(unsigned ichan)
{
  unsigned idxc;
  if (findPlaceInList(G_LIST_MODEL(store), idxc, [ichan](gpointer i) {
        if (D_CHANNEL_INFO(i)->channel == ichan)
          return 0;
        if (D_CHANNEL_INFO(i)->channel > ichan)
          return 1;
        return -1;
      })) {

    auto channel =
      D_CHANNEL_INFO(g_list_model_get_item(G_LIST_MODEL(store), idxc));
    auto elist = channel->sublist;
    if (!elist)
      return;
    for (guint idxe = 0U; idxe < g_list_model_get_n_items(G_LIST_MODEL(elist));
         idxe++) {
      auto entry =
        D_CHANNEL_INFO(g_list_model_get_item(G_LIST_MODEL(elist), idxe));
      auto edata = findInfo(infolist, ichan, entry->entry);
      d_channel_info_set_count(entry, edata->seq_id);

      auto rlist = entry->sublist;
      if (rlist) {
        for (guint idxr = 0U;
             idxr < g_list_model_get_n_items(G_LIST_MODEL(rlist)); idxr++) {
          auto reader =
            D_CHANNEL_INFO(g_list_model_get_item(G_LIST_MODEL(rlist), idxr));
          auto rdata = findInfo(infolist, ichan, reader->entry, reader->reader);
          d_channel_info_set_count(reader, rdata->seq_id);
        }
      }
    }
  }
}

void ChannelOverviewGtk4::showChanges()
{
  if (gtk_widget_get_visible(channel_window)) {
    gtk_widget_queue_draw(GTK_WIDGET(channel_tree));
  }
}

void ChannelOverviewGtk4::cbClose(GtkButton *button, gpointer gp)
{
  GtkDuecaView::toggleView(menuaction);
}

void ChannelOverviewGtk4::cbRefreshCounts(GtkButton *button, gpointer gp)
{
  refreshCounts();
}

gboolean ChannelOverviewGtk4::cbHide(GtkWidget *window, gpointer user_data)
{
  GtkDuecaView::toggleView(menuaction);
  return TRUE;
}

void ChannelOverviewGtk4::closeMonitor(unsigned ichan, unsigned ientry)
{
  auto entry = findInfo(infolist, ichan, ientry);
  if (entry) {
    entry->monitor->close();
    unsigned idxc, idxe;
    auto e = findEntry(ichan, ientry, idxc, idxe);
    if (e) {
      d_channel_info_set_monitor(e, FALSE);
    }
  }
}

void ChannelOverviewGtk4::monitorToggle(GtkCheckButton *btn, _DChannelInfo *ci)
{
  gboolean open = gtk_check_button_get_active(btn);
  auto entry = findInfo(infolist, ci->channel, ci->entry);
  if (entry) {

    auto &monitor = entry->monitor;

    if (open == TRUE) {
      if (!monitor) {
        monitor = new ChannelDataMonitorGtk4(this, ci->channel, ci->entry,
                                             monitor_gladefile);
      }
      monitor->open();
    }
    else {
      monitor->close();
    }
  }
}

void ChannelOverviewGtk4::cbSetupLabel(GtkSignalListItemFactory *fact,
                                       GtkListItem *item, gpointer user_data)
{
  auto label = gtk_label_new("");
  gtk_list_item_set_child(item, label);
  // g_object_unref(label);
}

void ChannelOverviewGtk4::cbSetupExpander(GtkSignalListItemFactory *fact,
                                          GtkListItem *item, gpointer user_data)
{
  auto label = gtk_label_new("");
  auto expander = gtk_tree_expander_new();
  gtk_tree_expander_set_child(GTK_TREE_EXPANDER(expander), label);
  gtk_list_item_set_child(item, expander);
  // g_object_unref(label);
  // g_object_unref(expander);
}

void ChannelOverviewGtk4::cbSetupImage(GtkSignalListItemFactory *fact,
                                       GtkListItem *item, gpointer user_data)
{
  auto image = gtk_image_new();
  gtk_list_item_set_child(item, image);
  // g_object_unref(image);
}

void ChannelOverviewGtk4_monitorToggle(GObject *w, gpointer self)
{
  auto row = g_object_get_data(w, "entry-row");
  auto ci = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (self && ci) {
    reinterpret_cast<ChannelOverviewGtk4 *>(self)->monitorToggle(
      GTK_CHECK_BUTTON(w), ci);
  }
}

void ChannelOverviewGtk4::cbSetupCheckbox(GtkSignalListItemFactory *fact,
                                          GtkListItem *item, gpointer user_data)
{
  auto checkbox = gtk_check_button_new();
  g_signal_connect(G_OBJECT(checkbox), "toggled",
                   G_CALLBACK(ChannelOverviewGtk4_monitorToggle),
                   gpointer(this));
  gtk_list_item_set_child(item, checkbox);
  // g_object_unref(checkbox);
}

void ChannelOverviewGtk4::cbBindChannelNum(GtkSignalListItemFactory *fact,
                                           GtkListItem *item,
                                           gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto channel = findInfo(infolist, chn->channel);
  if (chn->type == Channel && channel) {
    auto label = GTK_LABEL(gtk_list_item_get_child(item));
    gtk_label_set_label(
      label, boost::str(boost::format("%d") % channel->chanid).c_str());
    // g_object_unref(label);
  }
}

void ChannelOverviewGtk4::cbBindChannelName(GtkSignalListItemFactory *fact,
                                            GtkListItem *item,
                                            gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto channel = findInfo(infolist, chn->channel);
  if (chn->type == Channel && channel) {
    auto label = GTK_LABEL(gtk_list_item_get_child(item));
    gtk_label_set_label(label, channel->name.c_str());
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
  }
}

void ChannelOverviewGtk4::cbBindEntryNum(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto expander = GTK_TREE_EXPANDER(gtk_list_item_get_child(item));

  if (chn->type == Channel) {
    gtk_tree_expander_set_list_row(expander, GTK_TREE_LIST_ROW(row));
  }
  else if (chn->type == Entry) {

    auto entry = findInfo(infolist, chn->channel, chn->entry);
    if (!entry)
      return;
    auto label = GTK_LABEL(gtk_tree_expander_get_child(expander));
    gtk_label_set_label(
      label, boost::lexical_cast<std::string>(entry->wdata.entryid).c_str());
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
    gtk_widget_set_tooltip_text(GTK_WIDGET(label), entry->wdata.label.c_str());
  }
}

void ChannelOverviewGtk4::cbBindWriterId(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->type == Entry) {
    auto entry = findInfo(infolist, chn->channel, chn->entry);
    if (!entry)
      return;
    auto label = gtk_list_item_get_child(item);
    gtk_label_set_text(
      GTK_LABEL(label),
      boost::lexical_cast<std::string>(entry->wdata.clientid).c_str());
    gtk_widget_set_tooltip_text(
      GTK_WIDGET(label),
      ObjectManager::single()->getNameSet(entry->wdata.clientid).name.c_str());
  }
}

void ChannelOverviewGtk4::cbBindES(GtkSignalListItemFactory *fact,
                                   GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->type == Entry) {
    auto entry = findInfo(infolist, chn->channel, chn->entry);
    if (!entry)
      return;
    auto img = gtk_list_item_get_child(item);
    if (entry->wdata.eventtype) {
      gtk_image_set_from_paintable(GTK_IMAGE(img), GDK_PAINTABLE(event_icon));
    }
    else {
      gtk_image_set_from_paintable(GTK_IMAGE(img), GDK_PAINTABLE(stream_icon));
    }
    gtk_widget_set_tooltip_text(GTK_WIDGET(img),
                                entry->wdata.dataclass.c_str());
  }
}

gboolean uint2text(GBinding *bnd, const GValue *source, GValue *target,
                   gpointer user_data)
{
  g_value_set_string(
    target,
    boost::str(boost::format("%8d") % g_value_get_uint(source)).c_str());
  return TRUE;
}

void ChannelOverviewGtk4::cbBindNWrites(GtkSignalListItemFactory *fact,
                                        GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->type == Entry) {
    auto entry = findInfo(infolist, chn->channel, chn->entry);
    if (!entry)
      return;
    auto label = gtk_list_item_get_child(item);
    g_object_bind_property_full(chn, "rwcount", label, "label",
                                G_BINDING_DEFAULT, uint2text, NULL, NULL, NULL);
    d_channel_info_set_count(chn, entry->seq_id);
    /* gtk_label_set_label(
      GTK_LABEL(label),
      boost::lexical_cast<std::string>(entry->seq_id).c_str()); */
  }
}

void ChannelOverviewGtk4::cbBindReaderId(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto expander = GTK_TREE_EXPANDER(gtk_list_item_get_child(item));
  if (chn->type == Entry) {
    gtk_tree_expander_set_list_row(expander, GTK_TREE_LIST_ROW(row));
  }
  else if (chn->type == Reader) {
    auto reader = findInfo(infolist, chn->channel, chn->entry, chn->reader);
    if (!reader)
      return;
    auto label = gtk_tree_expander_get_child(expander);
    gtk_label_set_label(
      GTK_LABEL(label),
      boost::lexical_cast<std::string>(reader->readerid).c_str());
    gtk_widget_set_tooltip_text(
      GTK_WIDGET(label),
      ObjectManager::single()->getNameSet(reader->rdata.clientid).name.c_str());
  }
}

void ChannelOverviewGtk4::cbBindNReads(GtkSignalListItemFactory *fact,
                                       GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->type == Reader) {
    auto reader = findInfo(infolist, chn->channel, chn->entry, chn->reader);
    if (!reader)
      return;
    auto label = gtk_list_item_get_child(item);
    g_object_bind_property_full(chn, "rwcount", label, "label",
                                G_BINDING_DEFAULT, uint2text, NULL, NULL, NULL);
    d_channel_info_set_count(chn, reader->seq_id);
    // gtk_label_set_label(
    //   GTK_LABEL(label),
    //   boost::lexical_cast<std::string>(reader->seq_id).c_str());
  }
}

void ChannelOverviewGtk4::cbBindSel(GtkSignalListItemFactory *fact,
                                    GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->type == Reader) {
    auto reader = findInfo(infolist, chn->channel, chn->entry, chn->reader);
    if (!reader)
      return;
    auto img = gtk_list_item_get_child(item);
    gtk_image_set_from_paintable(
      GTK_IMAGE(img),
      select_icon[min(reader->rdata.action, ChannelReadInfo::byLabel)]);
  }
}

void ChannelOverviewGtk4::cbBindSeq(GtkSignalListItemFactory *fact,
                                    GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->type == Reader) {
    auto reader = findInfo(infolist, chn->channel, chn->entry, chn->reader);
    if (!reader)
      return;
    auto img = gtk_list_item_get_child(item);

    if (reader->rdata.sequential) {
      gtk_image_set_from_paintable(GTK_IMAGE(img), sequent_icon[0]);
    }
    else {
      gtk_image_set_from_paintable(GTK_IMAGE(img), sequent_icon[1]);
    }
  }
}

void ChannelOverviewGtk4::cbBindView(GtkSignalListItemFactory *fact,
                                     GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto toggle = gtk_list_item_get_child(item);
  if (chn->type == Entry) {

    auto entry = findInfo(infolist, chn->channel, chn->entry);
    if (!entry)
      return;

    // data property needs to point to the current row
    auto existing = g_object_get_data(G_OBJECT(toggle), "entry-row");
    if (existing != row) {
      g_object_set_data(G_OBJECT(toggle), "entry-row", row);
    }
    g_object_bind_property(chn, "monitor", toggle, "active",
                           G_BINDING_BIDIRECTIONAL);
    d_channel_info_set_monitor(chn, entry->monitor && entry->monitor->isOpen());
    // gtk_check_button_set_active(GTK_CHECK_BUTTON(toggle),
    //                              entry->monitor &&
    //                              entry->monitor->isOpen());
    gtk_widget_set_visible(toggle, TRUE);
  }
  else {
    gtk_widget_set_visible(toggle, FALSE);
  }
}

DUECA_NS_END
