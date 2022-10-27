/* ------------------------------------------------------------------   */
/*      item            : GtkGladeWindow.hxx
        made by         : Rene van Paassen
        date            : 051017
        category        : header file
        description     :
        documentation   : DUECA_API
        changes         : 051017 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GtkGladeWindow_hxx
#define GtkGladeWindow_hxx

#include <gtk/gtk.h>
#include <dueca_ns.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include "GtkCaller.hxx"
#include "GladeException.hxx"
#include <boost/any.hpp>

// Forward declaration
namespace Gtk {
  class Widget;
}

/** \file gtk3/GtkGladeWindow.hxx
    Gtk GUI facilities. */

DUECA_NS_START

class CommObjectReader;
class CommObjectWriter;

/** creation of a caller, 1 parameter and the gpointer parameter. */
template<class T, typename RET, typename P1>
GtkCaller* gtk_callback( RET (T:: *call) (P1, gpointer) )
{
  return new GtkCallerImp1<T,RET,P1>(call);
}

/** creation of a caller, 2 parameters and the gpointer parameter. */
template<class T, typename RET, typename P1, typename P2>
GtkCaller* gtk_callback( RET (T:: *call) (P1, P2, gpointer) )
{
  return new GtkCallerImp2<T,RET,P1,P2>(call);
}

/** creation of a caller, 3 parameters and the gpointer parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3>
GtkCaller* gtk_callback( RET (T:: *call) (P1, P2, P3, gpointer) )
{
  return new GtkCallerImp3<T,RET,P1,P2,P3>(call);
}

/** creation of a caller, 4 parameters and the gpointer parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4>
GtkCaller* gtk_callback( RET (T:: *call) (P1, P2, P3, P4, gpointer) )
{
  return new GtkCallerImp4<T,RET,P1,P2,P3,P4>(call);
}

/** creation of a caller, 5 parameters and the gpointer parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5>
GtkCaller* gtk_callback( RET (T:: *call) (P1, P2, P3, P4, P5, gpointer) )
{
  return new GtkCallerImp5<T,RET,P1,P2,P3,P4,P5>(call);
}

/** creation of a caller, 6 parameters and the gpointer parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6>
GtkCaller* gtk_callback( RET (T:: *call) (P1, P2, P3, P4, P5, P6, gpointer) )
{
  return new GtkCallerImp6<T,RET,P1,P2,P3,P4,P5,P6>(call);
}

/** creation of a caller, 7 parameters and the gpointer parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6, typename P7>
GtkCaller* gtk_callback( RET (T:: *call) (P1, P2, P3, P4, P5, P6, P7,
                                          gpointer) )
{
  return new GtkCallerImp7<T,RET,P1,P2,P3,P4,P5,P6,P7>(call);
}

/** creation of a caller, 8 parameters and the gpointer parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6, typename P7, typename P8>
GtkCaller* gtk_callback( RET (T:: *call) (P1, P2, P3, P4, P5, P6, P7, P8,
                                          gpointer) )
{
  return new GtkCallerImp8<T,RET,P1,P2,P3,P4,P5,P6,P7,P8>(call);
}

/** Structure that assembles a widget name, a callback function and
    the widget signal that should trigger the callback function. */
struct GladeCallbackTable {
  /** Widget name for linking callback */
  const char *widget;
  /** GTK signal name. */
  const char *signal;
  /** Function receiving the callback. Create this one with a
      gtk_callback() function. */
  GtkCaller* func;
  /** User data pointer. */
  gpointer user_data;
};

/** A GUI window directly from a glade interface file. Supply the
    interface file name, and a table of pointers to callback
    functions. As an example:
    \code
    class MyGui
    {
      // may be a member, may derive from it
      GtkGladeWindow mywindow;

    public:
      // constructor
      MyGui();

      // callback function
      void buttonPress(GtkWidget *w, gpointer gp);
    };

    // button press method of mygui:
    MyGui::buttonPress(GtkWidget *w, gpointer gp)
    {
      std::cout << "button pressed" << endl;
    }

    // table with callbacks
    static GladeCallbackTable table[] =
    {
      // links the button, with signal pressed, to ButtonPress
      { "mybutton", "pressed", gtk_callback(&MyGui::buttonPress),
        an_optional_gpointer_variable },
      { "otherbutton", "released", gtk_callback(&MyGui::buttonPress2) },
      // close off in familiar style
      { NULL, NULL, NULL, NULL }
    }

    // constructor, opens window and passes table, so callback is connected
    MyGui::MyGui() :
      mywindow()
    {
      // supply file and open window
      mywindow.readGladeFile("mywindow.glade", "thewindow",
                             reinterpret_cast<gpointer>(this),
                             table);

      mywindow.show();
    }
    \endcode

    You can most easily determine the signatures of the callback
    functions you use by letting glade generate c source code
    files. Stick to the signature generated by glade, since deviating
    from that signature may result in all kinds of unclean mess.

    The last argument of the callback function, the gpointer argument,
    is normally a pointer to user data that one can add to the signal
    connection in gtk. However, the callback system used by
    GtkGladeWindow already uses *that* gpointer. You can add a new
    value by specifying it in the last column of the
    GladeCallbackTable.
 */
class GtkGladeWindow
{
  /** A flag that avoids multiple initialization of glade. */
  static bool initialised_glade;

  /** A flag that avoids multiple initialization of gtkmm. */
  static bool initialised_gtkmm;

  /** The main window. */
  GtkWidget* window;

  /** Builder */
  GtkBuilder* builder;

  /** X coordinate of requested window position, -1 if default */
  int offset_x;

  /** Y coordinate of requested window position, -1 if default */
  int offset_y;

  /** width of requested, 0 if default */
  int size_x;

  /** heigth of requested, 0 if default */
  int size_y;

  /** A map of already initialized C++ widget objects, gtkmm interface */
  std::map<std::string, Gtk::Widget*> widgets;

  /** Helper, set a double value on a widget */
  bool _setValue(const char* wname, double value, bool warn);

  /** Helper, set a string value on a widget */
  bool _setValue(const char* wname, const char* value, bool warn);

  /** Helper, set a state on a widget */
  bool _setValue(const char* wname, bool value, bool warn);

  /** Helper, set any value on a widget */
  bool _setValue(const char* wname, const char* mname,
		 boost::any& b, bool warn);

  /** Helper, get a state from a widget */
  template<class T>
  bool __getValue(const char* wname, boost::any& alue, bool warn);

  /** Helper, get any value from a widget */
  bool _getValue(const char* wname, const char* klass, const char* mname,
		 boost::any& b, bool warn);

public:
  /** Constructor. */
  GtkGladeWindow();

  /** Destructor. Also closes the window if not already closed. */
  ~GtkGladeWindow();

  /** Initialization.
      \param file        Name of the glade interface file
      \param mainwidget  Top-level widget (normally a window) in that
                         file that will be opened.
      \param client      Pointer to the class that will be receiving
                         the callbacks.
      \param table       Table linking widget, signal, callback
                         function and optionally the pointer argument
                         to the callback function.
      @param connect_signals Connect gobject callback signals
      \returns           true if all OK. */
  bool readGladeFile(const char* file,
                     const char* mainwidget,
                     gpointer client = NULL,
                     const GladeCallbackTable *table =
                     reinterpret_cast<GladeCallbackTable*>(NULL),
                     bool connect_signals = false);

  /** Connect callbacks to widgets. You can repeatedly use this
      function, adding more callbacks to the widgets.
      \param client      Pointer to the class that will be receiving
                         the callbacks.
      \param table       Table linking widget, signal, callback
                         function and optionally the pointer argument
                         to the callback function. */
  void connectCallbacks(gpointer client, const GladeCallbackTable *table);

  /** Connect callbacks to widgets. Callbacks will be added as last.
      You can repeatedly use this
      function, adding more callbacks to the widgets.
      \param client      Pointer to the class that will be receiving
                         the callbacks.
      \param table       Table linking widget, signal, callback
                         function and optionally the pointer argument
                         to the callback function. */
  void connectCallbacksAfter(gpointer client, const GladeCallbackTable *table);

  /** Connect GObject in-code callbacks, note that this is effective
      only once, and called when connect_signals=true in the readGladeFile
      function. */
  void connectCallbackSymbols(gpointer user_data=NULL);

  /** Access the widgets in this interface.

      Most objects in the interface will be widgets; note that for
      GtkListStore and GtkTreeStore objects you need getObject().

      @param wname       Widget name.
      @returns           A widget object.
  */
  GtkWidget* operator [] (const char* wname);

  /** Access anything in the interface file as objects

      @param name        Object name.
      @returns           A GObject pointer.
  */
  GObject* getObject(const char* name);

  /** Open the window

      By default, this uses the main window/widget.

      @param widget  Optional, widget name to show other widget or window
  */
  void show(const char* widget=NULL);

  /** Close the window.

      By default, this uses the main window/widget.

      @param widget  Optional, widget name to hide other widget or
                     window.
  */
  void hide(const char* widget=NULL);

  /** Use a DCO object to set the state of the interface.

      Parses the DCO object, create an ID based on the name and
      possibly an element number and tries to find matching elements
      in the interface based on the format, then pushes the DCO values
      in the corresponding widgets, if possible.

      @param dco       Reader object
      @param format    Format string, to be written with sprintf,
                       use "%s" to insert the element name, e.g.,
		       "mywidgets_%s"
      @param arrformat Format string to be used when connecting to
                       an array element, e.g. "mywidgets_%s_%02d"
      @param warn      Warn if either an element is not found, or
                       widget and datatype do not match.
      @returns         The number of successfully set values
   */
  unsigned setValues(CommObjectReader& dco,
		     const char* format, const char* arrformat = NULL,
		     bool warn=false);

  /** Find the current state of the interface and push into a DCO object.

      Parses the DCO object, create an ID based on the name and
      possibly an element number and tries to find matching elements
      in the interface based on the format, then reads the interface state and
      sets the values in the DCO.

      @param dco       Writer object
      @param format    Format string, to be written with sprintf,
                       use "%s" to insert the element name, e.g.,
		       "mywidgets_%s"
      @param arrformat Format string to be used when connecting to
                       an array element, e.g. "mywidgets_%s_%02d"
      @param warn      Warn if either an element is not found, or
                       widget and datatype do not match.
      @returns         The number of successfully read values
   */
  unsigned getValues(CommObjectWriter& dco,
		     const char* format, const char* arrformat = NULL,
		     bool warn=false);

#if GTK_MAJOR_VERSION >= 2
  /** \brief Obtain gtk widget with name 'name' as a C++ object
    *
    * Usage example:
    * \code
        Gtk::Button* b = NULL;
        myGladeWindow.getWidget("myButton", b);
    * \endcode
    */
  template<typename T>
  T* getWidget(const std::string& name, T*& w);

  /** \brief Obtain gtk widget with name 'name' as a C++ object using
      a derived class
    *
    * Usage example:
    * \code
      MyDerivedButton* b = NULL;
      myGladeWindow.getWidgetDerived("myButton", b);
    * \endcode
    */
  template<typename T>
      T* getWidgetDerived(const std::string& name, T*& w);

  /** \brief Obtain gtk widget with name 'name' as a C++ object
    *
    * Usage example:
    * \code
      Gtk::Widget* b = myGladeWindow.getWidget("myButton");
    * \endcode
    */
  Gtk::Widget* getWidget(const std::string& name);

  /// Init function for gtkmm
  void initGtkMM();
#endif

  /** Change position and size of the window.
      \param  p    Vector with offset and size elements, p[0] offset
                   x. p[1] offset y, if either < 0, position hints are
                   ignored. p[2] width, p[3] height, if either <= 0,
                   size hints are ignored. Vector must have 2 or 4 elements. */
  inline void setWindow(const std::vector<int>& p)
  { if (p.size() == 2 || p.size() == 4) {offset_x = p[0]; offset_y = p[1]; }
    if (p.size() == 4) { size_x = p[2]; size_y = p[3]; }
  }
};

#if GTK_MAJOR_VERSION >= 2
template<typename T>
  T* GtkGladeWindow::getWidget(const std::string& name, T*& w)
{
  w = dynamic_cast<T*>(getWidget(name));
  return w;
}

template<typename T>
  T* GtkGladeWindow::getWidgetDerived(const std::string& name, T*& w)
{
  if (!initialised_gtkmm) initGtkMM();

  std::map<std::string, Gtk::Widget*>::iterator it = widgets.find(name);
  if (it != widgets.end()) {
    return dynamic_cast<T*>(it->second);
  } else {
    if (GtkWidget* cw = GTK_WIDGET
        (gtk_builder_get_object(builder, name.c_str()))) {
      w = new T(cw);
      widgets[name] = w;
      return w;
    } else {
      throw (GladeException("Widget \"" + name + "\" does not exist!"));
    }
  }
}
#endif

DUECA_NS_END

#endif
