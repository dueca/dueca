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

#include "ChannelOverview.hxx"
#include "ChannelReadInfo.hxx"
#include "gio/gio.h"
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
#define DEBPRINTLEVEL 2
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
struct _DChannelInfo
{
  GObject parent;
  std::shared_ptr<ChannelOverview::ChannelInfoSet> channel;
  std::shared_ptr<ChannelOverview::ChannelInfoSet::EntryInfoSet> entry;
  std::shared_ptr<ChannelOverview::ChannelInfoSet::EntryInfoSet::ReadInfoSet>
    reader;
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
  menuaction(NULL)
{
  //
}

static void d_channel_info_clear(DChannelInfo *ci)
{
  DEB("Clearing channel info " << reinterpret_cast<void*>(ci));
  ci->channel.reset();
  ci->entry.reset();
  ci->reader.reset();
  /* if (ci->sublist) {
    for (auto ii = g_list_model_get_n_items(G_LIST_MODEL(ci->sublist)); ii--; ) {
      d_channel_info_clear(D_CHANNEL_INFO(g_list_model_get_item(G_LIST_MODEL(ci->sublist), ii)));
    } 
    g_object_unref(ci->sublist);
    ci->sublist = NULL;
  } */
}

void d_channel_info_destroy(gpointer data)
{
  auto ci = D_CHANNEL_INFO(data);
  if (ci) d_channel_info_clear(ci);
}

DChannelInfo *
d_channel_info_new(std::shared_ptr<ChannelOverview::ChannelInfoSet> &channel)
{
  auto res = D_CHANNEL_INFO(g_object_new(d_channel_info_get_type(), NULL));
  g_object_set_data_full(G_OBJECT(res), "mydestroy", gpointer(res), d_channel_info_destroy);
  res->channel = channel;
  res->sublist = NULL;
  return res;
}

DChannelInfo *d_channel_info_new(
  std::shared_ptr<ChannelOverview::ChannelInfoSet::EntryInfoSet> &entry)
{
  auto res = D_CHANNEL_INFO(g_object_new(d_channel_info_get_type(), NULL));
  g_object_set_data_full(G_OBJECT(res), "mydestroy", gpointer(res), d_channel_info_destroy);
  res->entry = entry;
  res->sublist = NULL;
  return res;
}

DChannelInfo *d_channel_info_new(
  const std::shared_ptr<
    ChannelOverview::ChannelInfoSet::EntryInfoSet::ReadInfoSet> &reader)
{
  auto res = D_CHANNEL_INFO(g_object_new(d_channel_info_get_type(), NULL));
  g_object_set_data_full(G_OBJECT(res), "mydestroy", gpointer(res), d_channel_info_destroy);
  res->reader = reader;
  res->sublist = NULL;
  return res;
}

static void d_channel_info_class_init(DChannelInfoClass *klass)
{
  //
}

static void d_channel_info_init(DChannelInfo *self)
{
  // placement construction to get correct list initialization
  new (&(self->channel)) std::shared_ptr<ChannelOverview::ChannelInfoSet>;
  new (&(self->entry))
    std::shared_ptr<ChannelOverview::ChannelInfoSet::EntryInfoSet>;
  new (&(self->reader))
    std::shared_ptr<ChannelOverview::ChannelInfoSet::EntryInfoSet::ReadInfoSet>;
}

void sublist_destroyed(gpointer _ci, GObject* oldlist)
{
  auto ci = D_CHANNEL_INFO(_ci);
  DEB("Sublist destroy, clearing channel info " << _ci);
  ci->channel.reset();
  ci->entry.reset();
  ci->reader.reset();
  ci->sublist = NULL;
}

static GListModel *add_data_element(gpointer _item, gpointer user_data)
{
  auto item = D_CHANNEL_INFO(_item);
  // if this row has a channel, and it has entries, return these in a list
  if (item->channel) {
    auto lm = g_list_store_new(d_channel_info_get_type());
    for (auto &e : item->channel->entries) {
      auto listmem = d_channel_info_new(e);
      g_object_weak_ref(G_OBJECT(lm), sublist_destroyed, listmem);
      g_list_store_append(lm, gpointer(listmem));
    }
    item->sublist = lm;
    DEB("expand for entries, channel " << item->channel->chanid << " list "
                                       << reinterpret_cast<void *>(lm)
                                       << " check " << G_IS_LIST_STORE(lm));
    //g_object_ref(lm);


    return G_LIST_MODEL(lm);
  }

  // if this row has an entry, and that has readers, return in a list
  if (item->entry) {
    auto lm = g_list_store_new(d_channel_info_get_type());
    for (auto &r : item->entry->rdata) {
      g_list_store_append(lm, gpointer(d_channel_info_new(r)));
    }
    item->sublist = lm;
    DEB("expand for readers, entry " << item->entry->wdata.entryid << " list "
                                     << reinterpret_cast<void *>(lm)
                                     << " check " << G_IS_LIST_STORE(lm));
    g_object_weak_ref(G_OBJECT(lm), sublist_destroyed, _item);
    return G_LIST_MODEL(lm);
  }

  // leaf node, never anything to expand, null list.
  item->sublist = NULL;
  return G_LIST_MODEL(NULL);
}

static void remove_data_element(gpointer _elt)
{
  auto elt = D_CHANNEL_INFO(_elt);
  DEB("Removing expansion " << reinterpret_cast<void*>(elt));
  if (elt) {
    d_channel_info_clear(elt);
  }
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
  auto model = gtk_tree_list_model_new(G_LIST_MODEL(store), FALSE, FALSE,
                                       add_data_element, NULL, remove_data_element);
  auto selection = gtk_no_selection_new(G_LIST_MODEL(model));
  gtk_column_view_set_model(channel_tree, GTK_SELECTION_MODEL(selection));

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
ChannelOverviewGtk4::~ChannelOverviewGtk4()
{
  //
}

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

void ChannelOverviewGtk4::reflectChanges(unsigned ichan)
{
  // channels (should have) consecutive numbering corresponding to the store?
  unsigned idx;
  if (findPlaceInList(G_LIST_MODEL(store), idx, [ichan](gpointer i) {
        if (D_CHANNEL_INFO(i)->channel->chanid == ichan)
          return 0;
        if (D_CHANNEL_INFO(i)->channel->chanid > ichan)
          return 1;
        return -1;
      })) {

    // channel was found, stuff changed, splice the same thing back again here
    g_list_model_items_changed(G_LIST_MODEL(store), idx, 0, 0);
  }
  else {

    // new channel to the list at place idx
    auto citem =
      reinterpret_cast<gpointer>(d_channel_info_new(infolist[ichan]));
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
        if (D_CHANNEL_INFO(i)->channel->chanid == ichan)
          return 0;
        if (D_CHANNEL_INFO(i)->channel->chanid > ichan)
          return 1;
        return -1;
      })) {
    assert(0);
  }

  // find the channel info entry
  auto citem = D_CHANNEL_INFO(g_list_model_get_item(G_LIST_MODEL(store), idxc));

  // re-insert it at this place, to get a notify
  // g_object_ref(citem);
  // g_list_store_splice(store, idxc, 1, reinterpret_cast<gpointer *>(&citem),
  // 1); g_object_unref(citem);

  // now find the entry in the sublist.
  if (citem->sublist) {

    DEB("Changing entry in channel "
        << citem->channel->chanid << " list "
        << reinterpret_cast<void *>(citem->sublist));
    unsigned idxe;
    if (findPlaceInList(
          G_LIST_MODEL(citem->sublist), idxe, [ientry](gpointer i) {
            DEB("Entry pointer " << i);
            auto ei = D_CHANNEL_INFO(i);
            DEB("Pointer to entry info " << reinterpret_cast<void *>(ei));
            DEB("Entry use count " << ei->entry.use_count());
            if (!ei->entry) return 1; 
            if (ei->entry->wdata.entryid == ientry) return 0;
            if (ei->entry->wdata.entryid > ientry) return 1;
            return -1;
          })) {

      // entry may be changed or deleted?
      if (infolist[ichan]->entries[ientry].get()) {

        // changed
        g_list_model_items_changed(G_LIST_MODEL(citem->sublist), idxe, 0, 0);
      }
      else {

        // deleted
        auto eitem = g_list_model_get_item(G_LIST_MODEL(citem->sublist), idxe);
        d_channel_info_clear(D_CHANNEL_INFO(eitem));
        g_list_store_splice(citem->sublist, idxe, 1, NULL, 0);
      }
    }
    else {

      // new entry at location idxe
      auto eitem = reinterpret_cast<gpointer>(
        d_channel_info_new(infolist[ichan]->entries[ientry]));
      g_list_store_splice(citem->sublist, idxe, 0, &eitem, 1);
      g_object_unref(eitem);
    }
  }
  g_list_model_items_changed(G_LIST_MODEL(store), idxc, 0, 0);
}

void ChannelOverviewGtk4::reflectChanges(unsigned ichan, unsigned ientry,
                                         uint32_t ireader)
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
        if (D_CHANNEL_INFO(i)->channel->chanid == ichan)
          return 0;
        if (D_CHANNEL_INFO(i)->channel->chanid > ichan)
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
    if (!findPlaceInList(
          G_LIST_MODEL(citem->sublist), idxe, [ientry](gpointer i) {
            if (D_CHANNEL_INFO(i)->entry->wdata.entryid == ientry)
              return 0;
            if (D_CHANNEL_INFO(i)->entry->wdata.entryid > ientry)
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
                            if (D_CHANNEL_INFO(i)->reader->readerid == ireader)
                              return 0;
                            if (D_CHANNEL_INFO(i)->reader->readerid > ireader)
                              return 1;
                            return -1;
                          })) {

        if (itr == infolist[ichan]->entries[ientry]->rdata.end()) {

          // gone from the overview, remove also from the reader list
          auto ritem =
            g_list_model_get_item(G_LIST_MODEL(eitem->sublist), idxr);
          d_channel_info_clear(D_CHANNEL_INFO(ritem));
          g_list_store_splice(eitem->sublist, idxr, 1, NULL, 0);
        }
        else {

          // indicate change in the readers list
          g_list_model_items_changed(G_LIST_MODEL(eitem->sublist), idxr, 0, 0);
        }
      }
      else {

        // add to the list
        auto ritem = reinterpret_cast<gpointer>(d_channel_info_new(*itr));
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

void ChannelOverviewGtk4::reflectCounts()
{
  // gtk_widget_queue_draw(GTK_WIDGET(window["col_nwrites"]));
  // gtk_widget_queue_draw(GTK_WIDGET(window["col_nreads"]));
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
  infolist[ichan]->entries[ientry]->monitor->close();

  showChanges();
}

void ChannelOverviewGtk4::monitorToggle(GtkToggleButton *btn, _DChannelInfo *ci)
{
  gboolean open = gtk_toggle_button_get_active(btn);

  if (open == TRUE) {
    if (!ci->entry->monitor) {
      ci->entry->monitor = new ChannelDataMonitorGtk4(
        this, ci->entry->wdata.channelid.getObjectId(),
        ci->entry->wdata.entryid, monitor_gladefile);
    }
    ci->entry->monitor->open();
  }
  else {
    ci->entry->monitor->close();
  }

  // gtk_widget_queue_draw(GTK_WIDGET(btn));
  // g_signal_emit_by_name(btn, "notify");
  //  showChanges();
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
      GTK_TOGGLE_BUTTON(w), ci);
  }
}

void ChannelOverviewGtk4::cbSetupCheckbox(GtkSignalListItemFactory *fact,
                                          GtkListItem *item, gpointer user_data)
{
  auto checkbox = gtk_toggle_button_new();
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
  if (chn->channel) {
    auto label = GTK_LABEL(gtk_list_item_get_child(item));
    gtk_label_set_label(
      label, boost::str(boost::format("%d") % chn->channel->chanid).c_str());
    // g_object_unref(label);
  }
}

void ChannelOverviewGtk4::cbBindChannelName(GtkSignalListItemFactory *fact,
                                            GtkListItem *item,
                                            gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->channel) {
    auto label = GTK_LABEL(gtk_list_item_get_child(item));
    gtk_label_set_label(label, chn->channel->name.c_str());
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
  }
}

void ChannelOverviewGtk4::cbBindEntryNum(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto expander = GTK_TREE_EXPANDER(gtk_list_item_get_child(item));
  if (chn->channel && chn->channel->entries.size()) {
    gtk_tree_expander_set_list_row(expander, GTK_TREE_LIST_ROW(row));
  }
  else if (chn->entry) {
    auto label = GTK_LABEL(gtk_tree_expander_get_child(expander));
    gtk_label_set_label(
      label,
      boost::lexical_cast<std::string>(chn->entry->wdata.entryid).c_str());
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
    gtk_widget_set_tooltip_text(GTK_WIDGET(label),
                                chn->entry->wdata.label.c_str());
  }
}

void ChannelOverviewGtk4::cbBindWriterId(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->entry) {
    auto label = gtk_list_item_get_child(item);
    gtk_label_set_text(
      GTK_LABEL(label),
      boost::lexical_cast<std::string>(chn->entry->wdata.clientid).c_str());
    // gtk_widget_set_tooltip_text(label, chn->entry->wdata.clientid);
  }
}

void ChannelOverviewGtk4::cbBindES(GtkSignalListItemFactory *fact,
                                   GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->entry) {
    auto img = gtk_list_item_get_child(item);
    if (chn->entry->wdata.eventtype) {
      gtk_image_set_from_paintable(GTK_IMAGE(img), GDK_PAINTABLE(event_icon));
    }
    else {
      gtk_image_set_from_paintable(GTK_IMAGE(img), GDK_PAINTABLE(stream_icon));
    }
  }
}

void ChannelOverviewGtk4::cbBindNWrites(GtkSignalListItemFactory *fact,
                                        GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->entry) {
    auto label = gtk_list_item_get_child(item);
    gtk_label_set_label(
      GTK_LABEL(label),
      boost::lexical_cast<std::string>(chn->entry->seq_id).c_str());
  }
}

void ChannelOverviewGtk4::cbBindReaderId(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto expander = GTK_TREE_EXPANDER(gtk_list_item_get_child(item));
  if (chn->entry && chn->entry->rdata.size()) {
    gtk_tree_expander_set_list_row(expander, GTK_TREE_LIST_ROW(row));
  }
  else if (chn->reader) {
    auto label = gtk_tree_expander_get_child(expander);
    gtk_label_set_label(
      GTK_LABEL(label),
      boost::lexical_cast<std::string>(chn->reader->readerid).c_str());
  }
}

void ChannelOverviewGtk4::cbBindNReads(GtkSignalListItemFactory *fact,
                                       GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->reader) {
    auto label = gtk_list_item_get_child(item);
    gtk_label_set_label(
      GTK_LABEL(label),
      boost::lexical_cast<std::string>(chn->reader->seq_id).c_str());
  }
}

void ChannelOverviewGtk4::cbBindSel(GtkSignalListItemFactory *fact,
                                    GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->reader) {
    auto img = gtk_list_item_get_child(item);
    gtk_image_set_from_paintable(
      GTK_IMAGE(img),
      select_icon[min(chn->reader->rdata.action, ChannelReadInfo::byLabel)]);
  }
}

void ChannelOverviewGtk4::cbBindSeq(GtkSignalListItemFactory *fact,
                                    GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto chn = D_CHANNEL_INFO(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (chn->reader) {
    auto img = gtk_list_item_get_child(item);

    if (chn->reader->rdata.sequential) {
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
  if (chn->entry) {

    // data property needs to point to the current row
    auto existing = g_object_get_data(G_OBJECT(toggle), "entry-row");
    if (existing != row) {
      g_object_set_data(G_OBJECT(toggle), "entry-row", row);
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),
                                 chn->entry->monitor &&
                                   chn->entry->monitor->isOpen());
    gtk_widget_set_visible(toggle, TRUE);
  }
  else {
    gtk_widget_set_visible(toggle, FALSE);
  }
}

DUECA_NS_END
