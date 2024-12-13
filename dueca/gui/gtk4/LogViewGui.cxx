/* ------------------------------------------------------------------   */
/*      item            : LogViewGui.cxx
        made by         : Rene' van Paassen
        date            : 061206
        category        : body file
        description     :
        changes         : 061206 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "LogMessage.hxx"
#include "gtk/gtk.h"
#include <sstream>
#define LogViewGui_cxx
#include "LogViewGui.hxx"
#include "LogView.hxx"
#include <DuecaPath.hxx>
#include "GtkGladeWindow.hxx"
#include <dueca-conf.h>
#include <ActivityDescriptions.hxx>
#include "GtkDuecaView.hxx"
#include <boost/format.hpp>
#include <debug.h>

DUECA_NS_START

// type for logging
struct _DLogEntry
{
  GObject parent;
  LogMessage msg;
};

G_DECLARE_FINAL_TYPE(DLogEntry, d_log_entry, D, LOG_ENTRY, GObject);
G_DEFINE_TYPE(DLogEntry, d_log_entry, G_TYPE_OBJECT);

static void d_log_entry_class_init(DLogEntryClass *klass) {}
static void d_log_entry_init(DLogEntry *self) {}

static DLogEntry *d_log_entry_new(const LogMessage &m)
{
  auto res = D_LOG_ENTRY(g_object_new(d_log_entry_get_type(), NULL));
  // logmessage is a fixed-size type, assignment to uninitialized mem is possible.
  res->msg = m;
  return res;
}

// type for loglevel control
struct _DLogLevel
{
  GObject parent;
  LogCategory cat;
  std::vector<unsigned> level;
};

G_DECLARE_FINAL_TYPE(DLogLevel, d_log_level, D, LOG_LEVEL, GObject);
G_DEFINE_TYPE(DLogLevel, d_log_level, G_TYPE_OBJECT);

static void d_log_level_class_init(DLogLevelClass *klass) {}
static void d_log_level_init(DLogLevel *self) {}

static DLogLevel *d_log_level_new(const LogCategory &m, unsigned nnodes)
{
  auto res = D_LOG_LEVEL(g_object_new(d_log_level_get_type(), NULL));
  res->cat = m;
  new(&res->level) std::vector<unsigned>();
  res->level.resize(nnodes);
  for (auto n = nnodes; n--;)
    res->level[n] = 2;
  return res;
}

/* toolkit dependent set of info */
struct LogViewGui::GuiInfo
{
  /** Glade file interface definition. */
  std::string gladefile;

  /** Object that reads the glade file, and builds up the window. */
  GtkGladeWindow gwindow;

  /** Table with log results. */
  GtkWidget *table;

  /** List store for log results table */
  GListStore *table_store;

  /** The item we add to the menu of DUECA's main window. */
  GAction *menuaction;

  /** Table with log level controls. */
  GtkWidget *controltable;

  /** List store for log level table */
  GListStore *controltable_store;

  /** Option strings */
  GtkStringList *levels;

  /** Constructor */
  GuiInfo(const vstring &gladefile) :
    gladefile(gladefile),
    table(NULL),
    table_store(NULL),
    menuaction(NULL),
    controltable(NULL),
    controltable_store(NULL)
  {}
};

LogViewGui::LogViewGui(LogView *master, unsigned int nodes) :
  gui(*(new GuiInfo(DuecaPath::prepend("logview-gtk4.ui")))),
  master(master),
  nodes(nodes),
  maxrows(100),
  nrows(0)
{
  //
}

LogViewGui::~LogViewGui()
{
  //
}

static GladeCallbackTable cb_links[] = {
  { "close", "clicked", gtk_callback(&LogViewGui::closeView) },
  { "pause", "clicked", gtk_callback(&LogViewGui::pauseLogging) },
  { "play", "clicked", gtk_callback(&LogViewGui::playLogging) },
  { "log_view", "close-request", gtk_callback(&LogViewGui::deleteView) },

  // log table setup bindings
  { "log_time_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },
  { "log_num_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },
  { "log_class_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },
  { "log_line_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },
  { "log_node_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },
  { "log_actlevel_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },
  { "log_id_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },
  { "log_activity_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },
  { "log_message_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },

  // level table setup bindings
  { "ctr_memo_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },
  { "ctr_explain_fact", "setup", gtk_callback(&LogViewGui::cbSetupLabel) },

  // log table bind bindings
  { "log_time_fact", "bind", gtk_callback(&LogViewGui::cbBindLogTime) },
  { "log_num_fact", "bind", gtk_callback(&LogViewGui::cbBindLogNumber) },
  { "log_class_fact", "bind", gtk_callback(&LogViewGui::cbBindLogClass) },
  { "log_line_fact", "bind", gtk_callback(&LogViewGui::cbBindLogLineFile) },
  { "log_node_fact", "bind", gtk_callback(&LogViewGui::cbBindLogNode) },
  { "log_actlevel_fact", "bind",
    gtk_callback(&LogViewGui::cbBindLogActivityLevel) },
  { "log_id_fact", "bind", gtk_callback(&LogViewGui::cbBindLogModuleId) },
  { "log_activity_fact", "bind",
    gtk_callback(&LogViewGui::cbBindLogActivityName) },
  { "log_message_fact", "bind", gtk_callback(&LogViewGui::cbBindLogMessage) },

  // level table bind bindings
  { "ctr_memo_fact", "bind", gtk_callback(&LogViewGui::cbBindCatMEMO) },
  { "ctr_explain_fact", "bind", gtk_callback(&LogViewGui::cbBindCatExplain) },

  { NULL, NULL, NULL }
};

void LogViewGui::closeView(GtkButton *button, gpointer user_data)
{
  GtkDuecaView::toggleView(gui.menuaction);
}

gboolean LogViewGui::deleteView(GtkWidget *window, 
                                gpointer user_data)
{
  g_signal_emit_by_name(G_OBJECT(gui.menuaction), "activate", NULL);

  // with this, the click is handled. By preventing further handlers,
  // the window is not destroyed.
  return TRUE;
}

void LogViewGui::pauseLogging(GtkButton *button, gpointer user_data)
{
  master->pause(true);
}

void LogViewGui::playLogging(GtkButton *button, gpointer user_data)
{
  master->pause(false);
}

bool LogViewGui::open(unsigned int nrows)
{
  // maximum number of rows
  maxrows = nrows;

  // use the glade file to create the window
  bool res =
    gui.gwindow.readGladeFile(gui.gladefile.c_str(), "log_view",
                              reinterpret_cast<gpointer>(this), cb_links);

  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the base GUI. Check DUECA
       installation and paths.
    */
    E_CNF(" Opening " << gui.gladefile << " failed");
    return false;
  }

  // get the appropriate widgets
  gui.table = gui.gwindow["logtable"];
  gui.table_store = g_list_store_new(d_log_entry_get_type());
  auto selection = gtk_single_selection_new(G_LIST_MODEL(gui.table_store));
  gtk_column_view_set_model(GTK_COLUMN_VIEW(gui.table),
                            GTK_SELECTION_MODEL(selection));

  gui.controltable = gui.gwindow["controltable"];
  gui.controltable_store = g_list_store_new(d_log_level_get_type());
  auto sel2 = gtk_single_selection_new(G_LIST_MODEL(gui.controltable_store));
  gtk_column_view_set_model(GTK_COLUMN_VIEW(gui.controltable),
                            GTK_SELECTION_MODEL(sel2));

  // add a list for the options
  const char *list[] = { "debug", "info", "warn", "error", NULL };
  gui.levels = gtk_string_list_new(list);

  // add a column with factory for each of the nodes
  auto cbsetup = gtk_callback(&LogViewGui::cbSetupDropboxLevel);
  auto cbbind = gtk_callback(&LogViewGui::cbBindCatLevel);

  // run over the nodes
  for (auto node = 0L; node < nodes; node++) {

    // factory
    auto fact = gtk_signal_list_item_factory_new();
    auto cbs = gtk_caller_new(*cbsetup, this, gpointer(node));
    g_signal_connect(fact, "setup", cbs->callback(), cbs);
    auto cbb = gtk_caller_new(*cbbind, this, gpointer(node));
    g_signal_connect(fact, "bind", cbb->callback(), cbb);

    // column
    auto column = gtk_column_view_column_new(
      boost::str(boost::format("node %d") % node).c_str(), fact);
    gtk_column_view_append_column(GTK_COLUMN_VIEW(gui.controltable), column);

    // release
    //g_object_unref(fact);
    //g_object_unref(column);
  }
  delete cbbind;
  delete cbsetup;

  // request the DuecaView object to make an entry for my window,
  // opening it on activation
  gui.menuaction= GtkDuecaView::single()->requestViewEntry("errorlog",
    "Error Log View", GTK_WIDGET(gui.gwindow["log_view"]));

  return true;
}

void LogViewGui::appendItem(const LogMessage &msg)
{
  auto item = d_log_entry_new(msg);
  g_list_store_append(gui.table_store, item);
  g_object_unref(item);
}

void LogViewGui::appendLogCategory(const LogCategory &cat)
{
  auto item = d_log_level_new(cat, nodes);
  g_list_store_append(gui.controltable_store, item);
  // g_object_unref(item);
}

void LogViewGui::cbSetupLabel(GtkSignalListItemFactory *fact,
                              GtkListItem *object, gpointer user_data)
{
  auto label = gtk_label_new("");
  gtk_list_item_set_child(object, label);
  // g_object_unref(label);
}

static void LogViewGui_changeLevel(GtkDropDown *self, GParamSpecUInt *pspec,
                                   gpointer user_data)
{
  auto sel = gtk_drop_down_get_selected(self);
  auto *lvlrow = D_LOG_LEVEL(g_object_get_data(G_OBJECT(self), "d_row"));
  auto node = reinterpret_cast<unsigned long>(
    g_object_get_data(G_OBJECT(self), "d_node"));
  lvlrow->level[node] = sel;
  if (lvlrow)
  {
    reinterpret_cast<LogView*>(user_data)->setLevel(lvlrow->cat, node, sel);
  }
}

void LogViewGui::cbSetupDropboxLevel(GtkSignalListItemFactory *fact,
                                     GtkListItem *object, gpointer user_data)
{
  // create the dropdown
  auto dd = gtk_drop_down_new(G_LIST_MODEL(gui.levels), NULL);
  // user_data contains the node number, remember in the dropdown
  g_object_set_data(G_OBJECT(dd), "d_node", user_data);
  // connect callback, user data here give a pointer to this object
  g_signal_connect(dd, "notify::selected", G_CALLBACK(LogViewGui_changeLevel),
                   this->master);
  gtk_list_item_set_child(object, dd);
  // g_object_unref(dd);
}

  /** bind  log time */
void LogViewGui::cbBindLogTime(GtkSignalListItemFactory *fact,
                               GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
  auto entry = D_LOG_ENTRY(gtk_list_item_get_item(item));
  std::stringstream time;
  entry->msg.time.showtime(time);
  gtk_label_set_text(label, time.str().c_str());
}

void LogViewGui::cbBindLogNumber(GtkSignalListItemFactory *fact,
                                 GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto entry = D_LOG_ENTRY(gtk_list_item_get_item(item));
  gtk_label_set_text(
    label, boost::str(boost::format("%5d") % entry->msg.count).c_str());
}

void LogViewGui::cbBindLogClass(GtkSignalListItemFactory *fact,
                                GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto entry = D_LOG_ENTRY(gtk_list_item_get_item(item));
  auto point = LogPoints::single().getPoint(entry->msg.logpoint,
                                            entry->msg.context.parts.node);
  union {
    uint32_t i;
    char name[5];
  } catconv;
  catconv.name[4] = '\000';
  catconv.i = point.category;
  gtk_label_set_text(label, catconv.name);
}

void LogViewGui::cbBindLogLineFile(GtkSignalListItemFactory *fact,
                                   GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
  auto entry = D_LOG_ENTRY(gtk_list_item_get_item(item));
  auto point = LogPoints::single().getPoint(entry->msg.logpoint,
                                            entry->msg.context.parts.node);
  std::ostringstream logpoint;
  logpoint << point.fname << ":" << point.line;
  gtk_label_set_text(label, logpoint.str().c_str());
}

void LogViewGui::cbBindLogNode(GtkSignalListItemFactory *fact,
                               GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto entry = D_LOG_ENTRY(gtk_list_item_get_item(item));
  gtk_label_set_text(
    label, boost::str(boost::format("%2d") % int(entry->msg.context.parts.node))
             .c_str());
}

void LogViewGui::cbBindLogActivityLevel(GtkSignalListItemFactory *fact,
                                        GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto entry = D_LOG_ENTRY(gtk_list_item_get_item(item));
  gtk_label_set_text(label, boost::str(boost::format("%2d") %
                                       int(entry->msg.context.parts.manager))
                              .c_str());
}

void LogViewGui::cbBindLogModuleId(GtkSignalListItemFactory *fact,
                                   GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto entry = D_LOG_ENTRY(gtk_list_item_get_item(item));
  stringstream objectid;
  objectid << ActivityDescriptions::single()[entry->msg.context].owner;
  gtk_label_set_text(label, objectid.str().c_str());
}

void LogViewGui::cbBindLogActivityName(GtkSignalListItemFactory *fact,
                                       GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
  auto entry = D_LOG_ENTRY(gtk_list_item_get_item(item));
  gtk_label_set_text(
    label, ActivityDescriptions::single()[entry->msg.context].name.c_str());
}

void LogViewGui::cbBindLogMessage(GtkSignalListItemFactory *fact,
                                  GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
  auto entry = D_LOG_ENTRY(gtk_list_item_get_item(item));
  gtk_label_set_text(label, entry->msg.message.c_str());
}

void LogViewGui::cbBindCatMEMO(GtkSignalListItemFactory *fact,
                               GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto cat = D_LOG_LEVEL(gtk_list_item_get_item(item));
  gtk_label_set_text(label, cat->cat.getName());
}

void LogViewGui::cbBindCatExplain(GtkSignalListItemFactory *fact,
                                  GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto cat = D_LOG_LEVEL(gtk_list_item_get_item(item));
  gtk_label_set_text(label, cat->cat.getExplain().c_str());
}

void LogViewGui::cbBindCatLevel(GtkSignalListItemFactory *fact,
                                GtkListItem *item, gpointer user_data)
{
  auto drop = GTK_DROP_DOWN(gtk_list_item_get_child(item));
  auto cat = D_LOG_LEVEL(gtk_list_item_get_item(item));
  auto node = reinterpret_cast<unsigned long>(
    g_object_get_data(G_OBJECT(fact), "d_node"));
  g_object_set_data(G_OBJECT(drop), "d_row", cat);
  gtk_drop_down_set_selected(drop, cat->level[node]);
}

DUECA_NS_END
