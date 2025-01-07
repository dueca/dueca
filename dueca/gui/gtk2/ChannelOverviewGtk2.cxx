/* ------------------------------------------------------------------   */
/*      item            : ChannelOverviewGtk2.cxx
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


#define ChannelOverviewGtk2_cxx

// include the definition of the module class
#include "ChannelOverviewGtk2.hxx"

// include the debug writing header, by default, write warning and
// error messages
#include <debug.h>

// include additional files needed for your calculation here
#include "ChannelDataMonitorGtk2.hxx"
#include "GtkDuecaView.hxx"

#include <boost/lexical_cast.hpp>
#include <dueca/ObjectManager.hxx>
#include <debprint.h>
#include <dueca/DuecaPath.hxx>

// the standard package for DUECA, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca/dueca.h>

DUECA_NS_START

// class/module name
const char* const ChannelOverviewGtk2::classname = "channel-view";

// Parameters to be inserted
const ParameterTable* ChannelOverviewGtk2::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {

    { "count-delay",
      new VarProbe<_ThisModule_,unsigned>
      (&_ThisModule_::delay_countcollect),
      "Delay to wait before collecting a requested count" },

    { "glade-file",
      new VarProbe<_ThisModule_,std::string>
      (&_ThisModule_::gladefile),
      "Interface description (glade, gtkbuilder) for the channel view window" },

    { "glade-file-monitor",
      new VarProbe<_ThisModule_,std::string>
      (&_ThisModule_::monitor_gladefile),
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
    { NULL, NULL,
      "A module that presents an overview of channel access" }
  };

  return parameter_table;
}

// constructor
ChannelOverviewGtk2::ChannelOverviewGtk2(Entity* e, const char* part, const
                   PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  ChannelOverview(e, part, ps),

  // initialize the data you need in your simulation or process
  gladefile(DuecaPath::prepend("channel_overview.glade2")),
  monitor_gladefile(DuecaPath::prepend("channel_datamonitor.glade2")),
  window(),
  store(NULL),
  menuitem(NULL),
  iwindow()
{
  //
}

struct attributedata {
  const char* name;
  const gint column;
};

// organize
namespace {
  struct columndata {
    // name of the column
    const char* widgetname;
    // title above column
    const char* title;
    // cell renderer
    GtkCellRenderer *renderer;
    // sort the column?
    const bool sort;
    // expand the column
    const bool expand;
    // list of attributes
    const attributedata attribs[4];
  };

  static GdkPixbuf *stream_icon = NULL;
  static GdkPixbuf *event_icon = NULL;
  static GdkPixbuf *sequent_icon[2] = { NULL, NULL };
  static GdkPixbuf *select_icon[3] = { NULL, NULL, NULL };
};

static void ChannelOverviewGtk2_monitortoggle(GtkCellRendererToggle *cell,
                                              gchar *path_str,
                                              gpointer data)
{
  reinterpret_cast<ChannelOverviewGtk2*>(data)->monitorToggle(cell, path_str);
}

bool ChannelOverviewGtk2::complete()
{
  static GladeCallbackTable cb_table[] = {
    { "close", "clicked",
      gtk_callback(&_ThisModule_::cbClose) },
    { "refresh_times", "clicked",
      gtk_callback(&_ThisModule_::cbRefreshCounts) },
#if 0
    { "pause_times", "clicked", gtk_callback(&_ThisModule_::cbPauseTimes) },
    { "run_times", "clicked", gtk_callback(&_ThisModule_::cbRunTimes) },
    { "interval", "value-changed", gtk_callback(&_ThisModule_::cbInterval) },
#endif
    { "channel_use_view", "delete_event",
      gtk_callback(&_ThisModule_::cbDelete)},
    { "channel_overview", "motion_notify_event",
      gtk_callback(&_ThisModule_::cbHover)},
    { "channel_overview", "leave_notify_event",
      gtk_callback(&_ThisModule_::cbLeave)},
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

  res = window.readGladeFile
    (gladefile.c_str(), "channel_use_view",
     reinterpret_cast<gpointer>(this), cb_table);
  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the base GUI. Check DUECA
       installation and paths.
    */
    E_CNF("failed to open channel overview " << gladefile);
    return res;
  }

  GtkWidget *channeltree = window["channel_overview"];

  // channel view shows:
  store = gtk_tree_store_new
    (S_numcolumns,
     G_TYPE_UINT,     // 0 * channelnum
     G_TYPE_STRING,   // 1  name
     G_TYPE_UINT,     // 2 * entrynum
     G_TYPE_STRING,   // 3   - hover to get label
     G_TYPE_STRING,   // 4   writerid
     G_TYPE_STRING,   // 5   - hover to get name
     GDK_TYPE_PIXBUF, // 6   E/S
     G_TYPE_STRING,   // 7  - hover to get datatype
     G_TYPE_UINT,     // 8   writecount
     G_TYPE_STRING,   // 9 * readerid
     G_TYPE_STRING,   //10   - hover to get name
     G_TYPE_INT,      //11   readcount (relative to write)
     G_TYPE_UINT,     //12   creationid
     G_TYPE_BOOLEAN,  //13   chanentry flag
     G_TYPE_BOOLEAN,  //14   writeentry flag
     G_TYPE_BOOLEAN,  //15   readentry flag
     GDK_TYPE_PIXBUF, //16   numbered/label/multi
     GDK_TYPE_PIXBUF, //17   sequential/pick
     G_TYPE_BOOLEAN   //18   view open
     );

  gtk_tree_view_set_model(GTK_TREE_VIEW(channeltree),
                          GTK_TREE_MODEL(store));

  GtkCellRenderer *txtrenderer = gtk_cell_renderer_text_new();
  GtkCellRenderer *pxbrenderer = gtk_cell_renderer_pixbuf_new();
  GtkCellRenderer *tglrenderer = gtk_cell_renderer_toggle_new();
  g_signal_connect(G_OBJECT(tglrenderer), "toggled",
                   G_CALLBACK(ChannelOverviewGtk2_monitortoggle),
                   reinterpret_cast<gpointer>(this));


  // name, title, renderer, sort, expand
  // (attribute, column) x n
  static columndata cdata[] = {
    { "co_chanid", "chan #", txtrenderer, true, false,
      { { "text", S_channelnum}, { "visible", S_ischanentry }, { NULL,0 } } },
    { "co_channelname", "channel name", txtrenderer, true, true,
      { { "text", S_channelname }, { "visible", S_ischanentry }, { NULL,0 } } },
    { "co_entrynum", "entry #", txtrenderer, true, false,
      { { "text", S_entrynum }, { "visible", S_iswriteentry }, { NULL,0 } } },
    { "co_writerid", "writer id", txtrenderer, false, false,
      { { "text", S_writerid }, { "visible", S_iswriteentry }, { NULL,0 } } },
    { "co_es", "E/S", pxbrenderer, false, false,
      { { "pixbuf", S_ES}, { "visible", S_iswriteentry }, { NULL,0 } } },
    { "co_writecount", "#writes", txtrenderer, false, false,
      { { "text", S_writecount }, { "visible", S_iswriteentry }, { NULL,0 } } },
    { "co_readerid", "reader id", txtrenderer, false, false,
      { { "text", S_readerid }, { "visible", S_isreadentry }, { NULL,0 } } },
    { "co_readcount", "#reads", txtrenderer, false, false,
      { { "text", S_readcount }, { "visible", S_isreadentry }, { NULL,0 } } },
    { "co_selection", "sel", pxbrenderer, false, false,
      { { "pixbuf", S_selection }, { "visible", S_isreadentry }, { NULL,0 } } },
    { "co_sequential", "seq", pxbrenderer, false, false,
      { { "pixbuf", S_sequential }, { "visible", S_isreadentry },
        { NULL,0 } } },
    { "co_view", "view", tglrenderer, false, false,
      { { "active", S_viewopen }, { "activatable", S_iswriteentry },
        { "visible", S_iswriteentry }, { NULL,0 } } },
    { NULL, NULL, NULL, false, false, { { NULL, 0} } }
  };
  for (const struct columndata* cd = cdata; cd->widgetname != NULL; cd++) {
    GtkTreeViewColumn *col;
    col = gtk_tree_view_column_new_with_attributes
      (cd->title, cd->renderer, NULL);
    for (const struct attributedata* at = cd->attribs; at->name != NULL; at++) {
      gtk_tree_view_column_add_attribute
        (col, cd->renderer, at->name, at->column);
    }
    if (cd->sort) gtk_tree_view_column_set_sort_indicator (col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(channeltree), col);
    if (cd->expand) gtk_tree_view_column_set_expand(col, TRUE);
  }

  // allow sorting
  gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(channeltree), TRUE);


  if (stream_icon == NULL) {
    GError *error = NULL;
    event_icon = gdk_pixbuf_new_from_file
      (DuecaPath::prepend("pixmaps/event-logo.png").c_str(), &error);
    if (error) {
      /* DUECA UI.

         Cannot load icon pixbuf. Check DUECA installation. 
      */
      E_XTR("Could not load pixbuf " << error->message);
      g_error_free(error); error = NULL;
      return false;
    }
    stream_icon = gdk_pixbuf_new_from_file
      (DuecaPath::prepend("pixmaps/stream-logo.png").c_str(), &error);
    if (error) {
      /* DUECA UI.

         Cannot load icon pixbuf. Check DUECA installation. 
      */
      E_XTR("Could not load pixbuf " << error->message);
      g_error_free(error); error = NULL;
      return false;
    }
    select_icon[0] = gdk_pixbuf_new_from_file
      (DuecaPath::prepend("pixmaps/number-logo.png").c_str(), &error);
    if (error) {
      /* DUECA UI.

         Cannot load icon pixbuf. Check DUECA installation. 
      */
      E_XTR("Could not load pixbuf " << error->message);
      g_error_free(error); error = NULL;
      return false;
    }
    select_icon[1] = gdk_pixbuf_new_from_file
      (DuecaPath::prepend("pixmaps/label-logo.png").c_str(), &error);
    if (error) {
      /* DUECA UI.

         Cannot load icon pixbuf. Check DUECA installation. 
      */
      E_XTR("Could not load pixbuf " << error->message);
      g_error_free(error); error = NULL;
      return false;
    }
    select_icon[2] = gdk_pixbuf_new_from_file
      (DuecaPath::prepend("pixmaps/multi-logo.png").c_str(), &error);
    if (error) {
      /* DUECA UI.

         Cannot load icon pixbuf. Check DUECA installation. 
      */
      E_XTR("Could not load pixbuf " << error->message);
      g_error_free(error); error = NULL;
      return false;
    }
    sequent_icon[0] = gdk_pixbuf_new_from_file
      (DuecaPath::prepend("pixmaps/sequential-logo.png").c_str(), &error);
    if (error) {
      /* DUECA UI.

         Cannot load icon pixbuf. Check DUECA installation. 
      */
      E_XTR("Could not load pixbuf " << error->message);
      g_error_free(error); error = NULL;
      return false;
    }
    sequent_icon[1] = gdk_pixbuf_new_from_file
      (DuecaPath::prepend("pixmaps/picking-logo.png").c_str(), &error);
    if (error) {
      /* DUECA UI.

         Cannot load icon pixbuf. Check DUECA installation. 
      */
      E_XTR("Could not load pixbuf " << error->message);
      g_error_free(error); error = NULL;
      return false;
    }
  }
  // window.show();
  iwindow.init();

  menuitem = GTK_WIDGET
    (GtkDuecaView::single()->requestViewEntry
     ("Channel View", GTK_OBJECT(window["channel_use_view"])));


  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
ChannelOverviewGtk2::~ChannelOverviewGtk2()
{
  //
}

void ChannelOverviewGtk2::reflectChanges(unsigned ichan)
{
  GtkTreeIter itchan;

  // get the first channel in the tree (if any?)
  gboolean have_itchan = gtk_tree_model_get_iter_first
    (GTK_TREE_MODEL(store), &itchan);
  gint position = 0;
  guint chan_in_tree = -1;

  // find out where we are at
  if (have_itchan) {
    gboolean not_at_end = TRUE;

    gtk_tree_model_get(GTK_TREE_MODEL(store), &itchan, 0, &chan_in_tree, -1);
    while (ichan > chan_in_tree && not_at_end == TRUE) {
      position++;
      not_at_end = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &itchan);
      if (not_at_end) {
        gtk_tree_model_get
          (GTK_TREE_MODEL(store), &itchan, 0, &chan_in_tree, -1);
      }
    }
  }

  if (ichan != chan_in_tree) {

    // there is no entry yet for this channel,
    gtk_tree_store_insert(store, &itchan, NULL, position);

    gtk_tree_store_set
      (store, &itchan,
       0, ichan,  // channel number
       1, infolist[ichan]->name.c_str(), // channel name
       13, TRUE,                         // channel entry visible
       -1);
  }
  else {
    /* DUECA UI.

       Failure updating channel information, because it is already
       present in the list.
    */
    E_XTR("Channel overview logic failure, channel already in tree");
  }
}

void ChannelOverviewGtk2::reflectChanges(unsigned ichan, unsigned ientry)
{
  // was the entry already in the tree?
  GtkTreeIter itchan, itentry;

  // get the first channel in the tree
  gboolean have_itchan = gtk_tree_model_get_iter_first
    (GTK_TREE_MODEL(store), &itchan);
  assert(have_itchan == TRUE);

  // find channel number in tree
  gboolean not_at_end = TRUE;
  guint chan_in_tree;
  gtk_tree_model_get(GTK_TREE_MODEL(store), &itchan, 0, &chan_in_tree, -1);
  while (ichan > chan_in_tree && not_at_end == TRUE) {
    not_at_end = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &itchan);
    if (not_at_end) {
      gtk_tree_model_get
        (GTK_TREE_MODEL(store), &itchan, 0, &chan_in_tree, -1);
    }
  }
  assert(ichan == chan_in_tree);

  // one level deeper, writing entries
  gboolean have_itentry = gtk_tree_model_iter_children
    (GTK_TREE_MODEL(store), &itentry, &itchan);

  gint position = 0;
  guint entry_in_tree = -1;
  if (have_itentry) {
    gboolean not_at_end = TRUE;
    gtk_tree_model_get
      (GTK_TREE_MODEL(store), &itentry, 2, &entry_in_tree, -1);
    while (ientry > entry_in_tree && not_at_end == TRUE) {
      position++;
      not_at_end = gtk_tree_model_iter_next
        (GTK_TREE_MODEL(store), &itentry);
      if (not_at_end) {
        gtk_tree_model_get
          (GTK_TREE_MODEL(store), &itentry, 2, &entry_in_tree, -1);
      }
    }
  }

  if (ientry != entry_in_tree &&
      infolist[ichan]->entries[ientry].get()) {

    // new entry
    gtk_tree_store_insert(store, &itentry, &itchan, position);
    gtk_tree_store_set
      (store, &itentry,
       0, ichan,       // channel as ref
       2, ientry,      // entry no
       3, infolist[ichan]->entries[ientry]->wdata.label.c_str(), // label
       4, boost::lexical_cast<std::string>                 // client id
       (infolist[ichan]->entries[ientry]->wdata.clientid).c_str(),
       5, ObjectManager::single()->getNameSet              // client name
       (infolist[ichan]->entries[ientry]->wdata.clientid).name.c_str(),
       6, infolist[ichan]->entries[ientry]->wdata.eventtype ?
       event_icon : stream_icon, // eventtype
       7, infolist[ichan]->entries[ientry]->wdata.dataclass.c_str(), // datacls
       8, 0, // count, for now
       14, TRUE, // write entry visible
       -1);
  }
  else if (ientry == entry_in_tree &&
           infolist[ichan]->entries[ientry].get() == NULL) {
    gtk_tree_store_remove(store, &itentry);
  }
  else {
    /* DUECA UI.

       Failure setting entry information
    */
    E_XTR("Channel overview logic failure, entry " << ientry);
  }
}

struct match_readid
{
  const uint32_t readerid;
  match_readid(const uint32_t readerid) : readerid(readerid) { }
  bool operator()
  (const
   std::shared_ptr<ChannelOverview::ChannelInfoSet::
   EntryInfoSet::ReadInfoSet>& s)
  { return readerid == s->rdata.creationid; }
};

void ChannelOverviewGtk2::reflectChanges(unsigned ichan, unsigned ientry,
                                         uint32_t ireader, unsigned creationid)
{
  // was the entry already in the tree?
  GtkTreeIter itchan, itentry, itreader;

  // get the first channel in the tree
  gboolean have_itchan = gtk_tree_model_get_iter_first
    (GTK_TREE_MODEL(store), &itchan);
  assert(have_itchan == TRUE);

  // check that the channel number is in the tree, get iterator
  gboolean not_at_end = TRUE;
  guint chan_in_tree;
  gtk_tree_model_get(GTK_TREE_MODEL(store), &itchan, 0, &chan_in_tree, -1);
  while (ichan > chan_in_tree && not_at_end == TRUE) {
    not_at_end = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &itchan);
    if (not_at_end) {
      gtk_tree_model_get
        (GTK_TREE_MODEL(store), &itchan, 0, &chan_in_tree, -1);
    }
  }
  assert(ichan == chan_in_tree);

  // one level deeper, writing entries
  gboolean have_itentry = gtk_tree_model_iter_children
    (GTK_TREE_MODEL(store), &itentry, &itchan);
  assert(have_itentry == TRUE);

  // find entry number in tree, end at that iterator
  not_at_end = TRUE;
  guint entry_in_tree;
  gtk_tree_model_get
    (GTK_TREE_MODEL(store), &itentry, 2, &entry_in_tree, -1);
  while (ientry > entry_in_tree && not_at_end == TRUE) {
    not_at_end = gtk_tree_model_iter_next
      (GTK_TREE_MODEL(store), &itentry);
    if (not_at_end) {
      gtk_tree_model_get
        (GTK_TREE_MODEL(store), &itentry, 2, &entry_in_tree, -1);
    }
  }
  assert(ientry == entry_in_tree);

  // now find the reader
  gint position = 0;
  gboolean have_itreader = gtk_tree_model_iter_children
    (GTK_TREE_MODEL(store), &itreader, &itentry);
  guint reader_in_tree = -1;
  if (have_itreader) {
    gboolean not_at_end = TRUE;
    gtk_tree_model_get
      (GTK_TREE_MODEL(store), &itreader, 12, &reader_in_tree, -1);
    while (ireader > reader_in_tree && not_at_end == TRUE) {
      position++;
      not_at_end = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &itreader);
      if (not_at_end) {
        gtk_tree_model_get
          (GTK_TREE_MODEL(store), &itreader, 12, &reader_in_tree, -1);
      }
    }
  }
  auto re = std::find_if
    (infolist[ichan]->entries[ientry]->rdata.begin(),
     infolist[ichan]->entries[ientry]->rdata.end(),
     match_readid(ireader));

  if (ireader != reader_in_tree &&
      re != infolist[ichan]->entries[ientry]->rdata.end()) {

    // new entry
    gtk_tree_store_insert(store, &itreader, &itentry, position);
    gtk_tree_store_set
      (store, &itreader,
       0, ichan,
       2, ientry,
       9, boost::lexical_cast<std::string>                 // client id
       ((*re)->rdata.clientid).c_str(),
       10, ObjectManager::single()->getNameSet              // client name
       ((*re)->rdata.clientid).name.c_str(),
       11, 0, // count, for now
       12, (*re)->rdata.creationid,
       15, TRUE,
       16, select_icon[min(2, int((*re)->rdata.action))],
       17, (*re)->rdata.sequential ? sequent_icon[0] : sequent_icon[1],
       -1);
  }
  else if (ireader == reader_in_tree &&
           re == infolist[ichan]->entries[ientry]->rdata.end()) {

    // delete the entry
    gtk_tree_store_remove(store, &itreader);
  }
  else {
    /* DUECA UI.

       Failure updating entry information
    */
    E_XTR("Channel overview logic failure " << ireader);
  }
}

void ChannelOverviewGtk2::reflectCounts()
{
  // was the entry already in the tree?
  GtkTreeIter itchan, itentry, itreader;

  // find the first channel in the tree, and its channel number
  gboolean have_itchan = gtk_tree_model_get_iter_first
    (GTK_TREE_MODEL(store), &itchan);

  while(have_itchan) {

    // find the channel number from the tree
    guint ichan;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &itchan, 0, &ichan, -1);

    // find the writing entries in the channel
    gboolean have_itentry = gtk_tree_model_iter_children
      (GTK_TREE_MODEL(store), &itentry, &itchan);

    while (have_itentry == TRUE) {

      // find the entry id stashed in the tree
      guint ientry;
      gtk_tree_model_get
        (GTK_TREE_MODEL(store), &itentry, S_entrynum, &ientry, -1);

      // store the entry write count
      unsigned wcount = infolist[ichan]->entries[ientry]->seq_id;
      gtk_tree_store_set(store, &itentry, S_writecount, wcount, -1);

      // now find the readers associated to this entry
      gboolean have_itreader = gtk_tree_model_iter_children
        (GTK_TREE_MODEL(store), &itreader, &itentry);

      while (have_itreader == TRUE) {

        // get the reader's id, as stored in the tree
        guint ireader = -1;
        gtk_tree_model_get
          (GTK_TREE_MODEL(store), &itreader, S_creationid, &ireader, -1);

        // find the entry that matches this ireader no
        auto re = std::find_if
          (infolist[ichan]->entries[ientry]->rdata.begin(),
           infolist[ichan]->entries[ientry]->rdata.end(),
           match_readid(ireader));

        // store it in the tree
        gtk_tree_store_set(store, &itreader, S_readcount, (*re)->seq_id, -1);

        // find the next reader
        have_itreader = gtk_tree_model_iter_next
          (GTK_TREE_MODEL(store), &itreader);
      }

      // find the next entry
      have_itentry = gtk_tree_model_iter_next
          (GTK_TREE_MODEL(store), &itentry);
    }

    // find the next channel
    have_itchan = gtk_tree_model_iter_next
      (GTK_TREE_MODEL(store), &itchan);
  }
}

void ChannelOverviewGtk2::showChanges()
{
  gtk_widget_queue_draw(window["channel_overview"]);
}

void ChannelOverviewGtk2::cbClose(GtkButton* button, gpointer gp)
{
  g_signal_emit_by_name(G_OBJECT(menuitem), "activate", NULL);
}

void ChannelOverviewGtk2::cbRefreshCounts(GtkButton* button, gpointer gp)
{
  refreshCounts();
}

gboolean ChannelOverviewGtk2::
cbDelete(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
  g_signal_emit_by_name(G_OBJECT(menuitem), "activate", NULL);
  return TRUE;
}

gboolean ChannelOverviewGtk2::
cbHover(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  GtkTreePath *path;
  GtkTreeViewColumn *column;
  gint cellx, celly;
  GtkTreeView *treeview = GTK_TREE_VIEW(widget);
  gtk_tree_view_get_path_at_pos
    (treeview, event->x, event->y,
     &path, &column, &cellx, &celly);

  gint nidx = gtk_tree_path_get_depth(path);
  GtkTreeIter iter;
  gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);

  if (nidx == 2 && column == gtk_tree_view_get_column(treeview, 2)) {
    gchararray name;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, S_entrylabel, &name, -1);
    if (iwindow.text != name) {
      iwindow.update(name, event->x_root - 30, event->y_root + 12);
    }
    g_free(name);
  }
  else if (nidx == 2 && column == gtk_tree_view_get_column(treeview, 3)) {
    gchararray name;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, S_writername, &name, -1);
    if (iwindow.text != name) {
      iwindow.update(name, event->x_root - 30, event->y_root + 12);
    }
    g_free(name);
  }
  else if (nidx == 2 && column == gtk_tree_view_get_column(treeview, 4)) {
    gchararray name;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, S_dataclass, &name, -1);
    if (iwindow.text != name) {
      iwindow.update(name, event->x_root - 30, event->y_root + 12);
    }
    g_free(name);
  }
  else if (nidx == 3 && column == gtk_tree_view_get_column(treeview, 6)) {
    gchararray name;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, S_readername, &name, -1);
    if (iwindow.text != name) {
      iwindow.update(name, event->x_root - 30, event->y_root + 12);
    }
    g_free(name);
  }

  gtk_tree_path_free(path);

  return FALSE;
}


gboolean ChannelOverviewGtk2::
cbLeave(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
  iwindow.hide();

  return TRUE;
}

void ChannelOverviewGtk2::InfoWindow::init()
{
  infowindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  label = gtk_label_new(NULL);
  gtk_container_add(GTK_CONTAINER(infowindow), label);
  gtk_window_set_decorated(GTK_WINDOW(infowindow), FALSE);
  gtk_widget_show(label);
  visible = false;
}

void ChannelOverviewGtk2::InfoWindow::
update(const gchararray& txt, gint x, gint y)
{
  text = txt;
  gtk_label_set_text(GTK_LABEL(label), txt);
  gtk_window_move(GTK_WINDOW(infowindow), x, y);
  if (!visible) {
    gtk_widget_show(infowindow);
    visible = true;
  }
}

void ChannelOverviewGtk2::InfoWindow::hide()
{
  text = "";
  gtk_widget_hide(infowindow);
  visible = false;
}

void ChannelOverviewGtk2::closeMonitor(unsigned ichan, unsigned ientry)
{
  GtkTreeIter itchan, itentry;
  gboolean not_at_end = gtk_tree_model_get_iter_first
    (GTK_TREE_MODEL(store), &itchan);

  // check that the channel number is in the tree, get iterator
  guint chan_in_tree;
  gtk_tree_model_get(GTK_TREE_MODEL(store), &itchan, 0, &chan_in_tree, -1);
  while (ichan > chan_in_tree && not_at_end == TRUE) {
    not_at_end = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &itchan);
    if (not_at_end) {
      gtk_tree_model_get
        (GTK_TREE_MODEL(store), &itchan, 0, &chan_in_tree, -1);
    }
  }
  assert(ichan == chan_in_tree);

  // one level deeper, writing entries
  gboolean have_itentry = gtk_tree_model_iter_children
    (GTK_TREE_MODEL(store), &itentry, &itchan);
  assert(have_itentry == TRUE);

  // find entry number in tree, end at that iterator
  not_at_end = TRUE;
  guint entry_in_tree;
  gtk_tree_model_get
    (GTK_TREE_MODEL(store), &itentry, 2, &entry_in_tree, -1);
  while (ientry > entry_in_tree && not_at_end == TRUE) {
    not_at_end = gtk_tree_model_iter_next
      (GTK_TREE_MODEL(store), &itentry);
    if (not_at_end) {
      gtk_tree_model_get
        (GTK_TREE_MODEL(store), &itentry, 2, &entry_in_tree, -1);
    }
  }
  assert(ientry == entry_in_tree);
  gtk_tree_store_set(store, &itentry, S_viewopen, FALSE, -1);
  infolist[ichan]->entries[ientry]->monitor->close();

  showChanges();
}

void ChannelOverviewGtk2::monitorToggle(GtkCellRendererToggle *cell,
                                        gchar *path_str)
{
  GtkTreeIter  iter;
  gboolean enabled;
  guint chanid, entryid;

  DEB("tree path " << path_str);

  GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
  gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
  gtk_tree_model_get
    (GTK_TREE_MODEL(store), &iter, S_viewopen, &enabled,
     S_channelnum, &chanid, S_entrynum, &entryid, -1);
  gtk_tree_path_free(path);

  // toggle the switch
  enabled = (enabled == TRUE) ? FALSE : TRUE;

  if (chanid > infolist.size() || infolist[chanid].get() == NULL ||
      entryid > infolist[chanid]->entries.size() ||
      infolist[chanid]->entries[entryid].get() == NULL) {
    /* DUECA UI.

       When trying to toggle the data monitoring of a channel entry,
       could not find the entry. May happen when the entry has been
       removed. */
    W_STS("Cannot get channel " << chanid << " entry " << entryid);
    return;
  }

  if (enabled == TRUE) {
    if (infolist[chanid]->entries[entryid]->monitor == NULL) {
      infolist[chanid]->entries[entryid]->monitor = new
        ChannelDataMonitorGtk2(this, infolist[chanid]->name,
                               chanid, entryid, monitor_gladefile);
    }
    infolist[chanid]->entries[entryid]->monitor->open();
  }
  else {
    infolist[chanid]->entries[entryid]->monitor->close();
  }
  gtk_tree_store_set(store, &iter, S_viewopen, enabled, -1);

  showChanges();
}

DUECA_NS_END



