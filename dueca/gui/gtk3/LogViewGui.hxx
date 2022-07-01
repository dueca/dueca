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
  LogView* master;

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
  LogViewGui(LogView* master, unsigned int nodes);

  /** Destructor. */
  ~LogViewGui();

  /** Open the window. */
  bool open(unsigned int nrows);

  /** Add a log item. */
  void appendItem(const LogMessage& msg);

  /** Add a log class. */
  void appendLogCategory(const LogCategory& cat);

  /** callback, close the view */
  void closeView(GtkButton *button, gpointer user_data);

  /** callback, close the view on deletion by window manager. */
  gboolean deleteView(GtkWidget *window, GdkEvent *event, gpointer user_data);

  /** callback, stop logging. */
  void pauseLogging(GtkButton *button, gpointer user_data);

  /** callback, play again. */
  void playLogging(GtkButton *button, gpointer user_data);

  /** Callback, updated logging level. */
  void editedLevel(GtkCellRendererText* renderer, gchar* pathstring,
                   gchar *new_text, gpointer user_data);
};

DUECA_NS_END

#endif
