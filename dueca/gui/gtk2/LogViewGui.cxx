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


#define LogViewGui_cxx
#include "LogViewGui.hxx"
#include "LogView.hxx"
#include <DuecaPath.hxx>
#include "GtkGladeWindow.hxx"
#include <dueca-conf.h>
#include <ActivityDescriptions.hxx>
#include "GtkDuecaView.hxx"

#include <debug.h>

DUECA_NS_START


struct LogViewGui::GuiInfo
{
  /** Glade file interface definition. */
  std::string       gladefile;

  /** Object that reads the glade file, and builds up the window. */
  GtkGladeWindow    gwindow;

  /** Table with log results. */
  GtkWidget*        table;

  /** List store for log results table */
  GtkListStore*     table_store;

  /** The item we add to the menu of DUECA's main window. */
  GtkWidget*        menuitem;

  /** Table with log level controls. */
  GtkWidget*        controltable;

  /** List store for log level table */
  GtkListStore*     controltable_store;

  /** Constructor */
  GuiInfo(const vstring& gladefile) :
    gladefile(gladefile),
    table(NULL),
    table_store(NULL),
    menuitem(NULL),
    controltable(NULL),
    controltable_store(NULL)
  { }
};

LogViewGui::LogViewGui(LogView* master, unsigned int nodes) :
  gui(*(new GuiInfo(DuecaPath::prepend("logview.glade2")))),
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
  { "close", "clicked", gtk_callback(&LogViewGui::closeView)},
  { "pause", "clicked", gtk_callback(&LogViewGui::pauseLogging)},
  { "play",  "clicked", gtk_callback(&LogViewGui::playLogging)},
  { "log_view", "delete_event", gtk_callback(&LogViewGui::deleteView)},
  { NULL, NULL, NULL }
};

void LogViewGui::closeView(GtkButton *button, gpointer user_data)
{
  g_signal_emit_by_name(G_OBJECT(gui.menuitem), "activate", NULL);
}

gboolean LogViewGui::deleteView(GtkWidget *window, GdkEvent *event,
                                gpointer user_data)
{
  g_signal_emit_by_name(G_OBJECT(gui.menuitem), "activate", NULL);

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

// a hand-used table
static GladeCallbackTable cb_renderer[] = {
  { NULL, "signal::edited", gtk_callback(&LogViewGui::editedLevel) }
};

bool LogViewGui::open(unsigned int nrows)
{
  // maximum number of rows
  maxrows = nrows;

  // use the glade file to create the window
  bool res = gui.gwindow.readGladeFile
    (gui.gladefile.c_str(), "log_view",
     reinterpret_cast<gpointer>(this), cb_links);

  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the base GUI. Check DUECA
       installation and paths.
    */
    E_CNF("Opening " << gui.gladefile << " failed");
    return false;
  }

  // get the appropriate widgets
  gui.table = gui.gwindow["logtable"];
  gui.table_store = gtk_list_store_new
    (9,
     G_TYPE_STRING,   // time
     G_TYPE_INT,      // repeat
     G_TYPE_STRING,   // message category
     G_TYPE_STRING,   // file + line
     G_TYPE_INT,      // node
     G_TYPE_INT,      // manager (-1 for none)
     G_TYPE_STRING,   // object id
     G_TYPE_STRING,   // activity
     G_TYPE_STRING);  // message

  // data on columns, renderers, etcetera
  struct ColData {
    int width;
    const char* name;
  };
  ColData coldata[] = {
    {  30, "time" },
    {  15, "#" },
    {  20, "class" },
    { 120, "file/line" },
    {  10, "N" },
    {  10, "A" },
    {  30, "id" },
    {  60, "activity" },
    { 200, "message" },
    {   0, NULL } };
  ColData *cdp = coldata;

  // define column names and renderer
  int icol = 0;
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  for (; cdp->name != NULL; cdp++) {
    GtkTreeViewColumn *col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, cdp->name);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui.table), col);
    gtk_tree_view_column_set_min_width(col, cdp->width);
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "text", icol++);
  }

  // supply model to the view
  gtk_tree_view_set_model(GTK_TREE_VIEW(gui.table),
                          GTK_TREE_MODEL(gui.table_store));

  // control table store.
  // pointer-to-logclass, memo text, log class text,

  gui.controltable = gui.gwindow["controltable"];
  GType types[nodes+3];
  for (int ii = nodes+3; ii--; ) types[ii] = G_TYPE_STRING;
  types[0] = G_TYPE_POINTER;
  gui.controltable_store = gtk_list_store_newv(nodes+3, types);

  // column 1 with category mnemonic
  {
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes
      ("MEMO", renderer, "text" , 1, NULL);
    gtk_tree_view_column_set_min_width(col, 40);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui.controltable), col);
  }

  // column 2 with log class
  {
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes
      ("Class", renderer, "text" , 2, NULL);
    gtk_tree_view_column_set_min_width(col, 40);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui.controltable), col);
  }

  // since gtk 2.6 there is a special combobox renderer
  GtkListStore* level = gtk_list_store_new(1, G_TYPE_STRING);
  const char* list[] =
    {"debug", "info", "warn", "error", NULL};
  for (const char** l = list; *l != NULL; l++) {
    GtkTreeIter iter; gtk_list_store_append(level, &iter);
    gtk_list_store_set(level, &iter, 0, *l, -1);
  }

  // column 3 and further with node controls
  // this is not available in all gtk 2 versions
#ifdef GTK_TYPE_CELL_RENDERER_COMBO
  for (unsigned int ii = 0; ii < nodes; ii++) {
    char buf[8]; snprintf(buf, sizeof(buf), "node %2d", ii);
    GtkCellRenderer *comborenderer = gtk_cell_renderer_combo_new();
    g_object_set(G_OBJECT(comborenderer),
                 "model", level,
                 "text-column", 0,
                 "editable", TRUE,
                 "has-entry", FALSE, NULL);
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes
      (buf, comborenderer, "text", ii+3, NULL);
    gtk_tree_view_column_set_min_width(col, 40);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui.controltable), col);
    GtkCaller* caller = cb_renderer->func->clone(this);
    caller->setGPointer(reinterpret_cast<gpointer>(ii));
    g_object_connect(G_OBJECT(comborenderer), cb_renderer->signal,
                     caller->callback(), caller, NULL);
  }
#else
#warning "GtkCellRendererCombo not defined, interface incomplete"
#endif

  // supply model to the view
  gtk_tree_view_set_model(GTK_TREE_VIEW(gui.controltable),
                          GTK_TREE_MODEL(gui.controltable_store));

  // request the DuecaView object to make an entry for my window,
  // opening it on activation
  gui.menuitem = GTK_WIDGET
    (GtkDuecaView::single()->requestViewEntry
     ("Error Log View", GTK_OBJECT(gui.gwindow["log_view"])));

  return true;
}

void LogViewGui::appendItem(const LogMessage& msg)
{
  // first re-format some stuff to strings
  ostringstream time, category, logpoint, objectid, activity;

  msg.time.show(time);     // time
  union { uint32_t i; char name[5]; } catconv; catconv.name[4] = '\000';
  const LogPoint &point = LogPoints::single().getPoint
    (msg.logpoint, msg.context.parts.node);
  catconv.i = point.category;
  category << point.level << catconv.name;
  logpoint << point.fname << ":" <<  point.line;
  objectid << ActivityDescriptions::single()[msg.context].owner;

  // Access the next row
  GtkTreeIter iter;
  gtk_list_store_append(gui.table_store, &iter);

  // push in the data
  gtk_list_store_set(gui.table_store, &iter,
                     0, time.str().c_str(), 1, msg.count,
                     2, category.str().c_str(), 3, logpoint.str().c_str(),
                     4, msg.context.parts.node, 5, msg.context.parts.manager,
                     6, objectid.str().c_str(),
                     7, ActivityDescriptions::single()[msg.context].name.c_str(),
                     8, msg.message.c_str(), -1);

  // limit maximum number of rows.
  if (maxrows && ++nrows > maxrows) {
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gui.table_store), &iter);
    gtk_list_store_remove(gui.table_store, &iter);
    nrows--;
  }
}


void LogViewGui::appendLogCategory(const LogCategory& cat)
{
  GtkTreeIter iter;
  gtk_list_store_append(gui.controltable_store, &iter);
  gtk_list_store_set(gui.controltable_store, &iter,
                     0, &cat, 1, cat.getName(),
                     2, cat.getExplain().c_str(), -1);
#ifdef GTK_TYPE_CELL_RENDERER_COMBO
  for (unsigned ii = 3; ii < nodes+3; ii++) {
    gtk_list_store_set(gui.controltable_store, &iter,
                       ii, "warn", -1);
  }
#endif
}

void LogViewGui::editedLevel(GtkCellRendererText* renderer,
                             gchar* pathstring,
                             gchar *new_text, gpointer user_data)
{
  GtkTreeIter iter;
  union { gpointer data; long column; } conv; conv.data = user_data;
  gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gui.controltable_store),
                                      &iter, pathstring);
  gtk_list_store_set(gui.controltable_store, &iter, conv.column + 3,
                     new_text, -1);

  // obtain corresponding category
  LogCategory *cat;
  gtk_tree_model_get(GTK_TREE_MODEL(gui.controltable_store),
                     &iter, 0, &cat, -1);

  // allow changes to be realised.
  master->setLevel(cat, conv.column, new_text);
}

DUECA_NS_END
