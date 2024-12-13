/* ------------------------------------------------------------------   */
/*      item            : LogViewGui.hxx
        made by         : Rene van Paassen
        date            : 061206
        category        : header file
        description     :
        changes         : 061206 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef LogViewGui_hxx
#define LogViewGui_hxx

#include <dueca_ns.h>
#include <LogMessage.hxx>
#include <LogCategory.hxx>
#include <LogPoints.hxx>
#include <ActivityDescriptions.hxx>
#include <gtk/gtk.h>

DUECA_NS_START

class LogView;

/** Interface for logging/reporting. */
class LogViewGui
{
  /** Advance declaration, captures the gui parameters. */
  struct GuiInfo;

  /** Gui parameters. */
  GuiInfo &gui;

  /** Pointer to the logview master, which will be called when buttons
      are clicked. */
  LogView *master;

  /** Number of nodes to consider. */
  unsigned int nodes;

  /** Maximum number of rows. */
  unsigned int maxrows;

  /** Current number of rows. */
  unsigned int nrows;

public:
  /** Constructor.
      \param master    Master of this window, will be called with
                       button clicks
      \param nodes     Number of nodes in this simulation
  */
  LogViewGui(LogView *master, unsigned int nodes);

  /** Destructor. */
  ~LogViewGui();

  /** Open the window. */
  bool open(unsigned int nrows);

  /** Add a log item. */
  void appendItem(const LogMessage &msg);

  /** Add a log class. */
  void appendLogCategory(const LogCategory &cat);

  /** callback, close the view */
  void closeView(GtkButton *button, gpointer user_data);

  /** callback, close the view on deletion by window manager. */
  gboolean deleteView(GtkWidget *window, gpointer user_data);

  /** callback, stop logging. */
  void pauseLogging(GtkButton *button, gpointer user_data);

  /** callback, play again. */
  void playLogging(GtkButton *button, gpointer user_data);

  /** Callback, updated logging level. */
  void editedLevel(GtkCellRendererText *renderer, gchar *pathstring,
                   gchar *new_text, gpointer user_data);

  /** set up a label item widget */
  void cbSetupLabel(GtkSignalListItemFactory *fact, GtkListItem *object,
                    gpointer user_data);

  /** set up a dropbox level item widget*/
  void cbSetupDropboxLevel(GtkSignalListItemFactory *fact, GtkListItem *object,
                           gpointer user_data);

  /** bind  log time */
  void cbBindLogTime(GtkSignalListItemFactory *fact, GtkListItem *object,
                     gpointer user_data);

  /** bind log number */
  void cbBindLogNumber(GtkSignalListItemFactory *fact, GtkListItem *object,
                       gpointer user_data);

  /** bine log class */
  void cbBindLogClass(GtkSignalListItemFactory *fact, GtkListItem *object,
                       gpointer user_data);

  /** line and file information */
  void cbBindLogLineFile(GtkSignalListItemFactory *fact, GtkListItem *object,
                         gpointer user_data);

  /** log message node */
  void cbBindLogNode(GtkSignalListItemFactory *fact, GtkListItem *object,
                     gpointer user_data);

  /** bind a log table column */
  void cbBindLogActivityLevel(GtkSignalListItemFactory *fact,
                              GtkListItem *object, gpointer user_data);

  /** bind a log table column */
  void cbBindLogModuleId(GtkSignalListItemFactory *fact, GtkListItem *object,
                         gpointer user_data);

  /** bind a log table column */
  void cbBindLogActivityName(GtkSignalListItemFactory *fact,
                             GtkListItem *object, gpointer user_data);

  /** bind a log table column */
  void cbBindLogMessage(GtkSignalListItemFactory *fact, GtkListItem *object,
                        gpointer user_data);

  /** bind a log level control column */
  void cbBindCatMEMO(GtkSignalListItemFactory *fact, GtkListItem *object,
                       gpointer user_data);

  /** bind a category explanation control column */
  void cbBindCatExplain(GtkSignalListItemFactory *fact, GtkListItem *object,
                        gpointer user_data);
                        
 /** bind a log level control column */
  void cbBindCatLevel(GtkSignalListItemFactory *fact, GtkListItem *object,
                        gpointer user_data);
};

DUECA_NS_END

#endif
