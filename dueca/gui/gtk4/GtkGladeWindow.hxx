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
#include "GtkCaller.hxx"
#include "GladeException.hxx"
#include <boost/any.hpp>

#define NO_COMBOBOX

// Forward declaration
namespace Gtk {
class Widget;
}

/** \file gtk4/GtkGladeWindow.hxx
    Gtk GUI facilities. */

DUECA_NS_START

class CommObjectReader;
class CommObjectWriter;
class ElementWriter;
class ElementReader;

/** creation of a caller, 1 parameter and the gpointer parameter. */
template <class T, typename RET, typename P1>
GtkCaller *gtk_callback(RET (T::*call)(P1, gpointer), gpointer obj = NULL)
{
  return new GtkCallerImp1<T, RET, P1>(call, obj);
}

/** creation of a caller, 2 parameters and the gpointer parameter. */
template <class T, typename RET, typename P1, typename P2>
GtkCaller *gtk_callback(RET (T::*call)(P1, P2, gpointer), gpointer obj = NULL)
{
  return new GtkCallerImp2<T, RET, P1, P2>(call, obj);
}

/** creation of a caller, 3 parameters and the gpointer parameter. */
template <class T, typename RET, typename P1, typename P2, typename P3>
GtkCaller *gtk_callback(RET (T::*call)(P1, P2, P3, gpointer),
                        gpointer obj = NULL)
{
  return new GtkCallerImp3<T, RET, P1, P2, P3>(call, obj);
}

/** creation of a caller, 4 parameters and the gpointer parameter. */
template <class T, typename RET, typename P1, typename P2, typename P3,
          typename P4>
GtkCaller *gtk_callback(RET (T::*call)(P1, P2, P3, P4, gpointer),
                        gpointer obj = NULL)
{
  return new GtkCallerImp4<T, RET, P1, P2, P3, P4>(call, obj);
}

/** creation of a caller, 5 parameters and the gpointer parameter. */
template <class T, typename RET, typename P1, typename P2, typename P3,
          typename P4, typename P5>
GtkCaller *gtk_callback(RET (T::*call)(P1, P2, P3, P4, P5, gpointer),
                        gpointer obj = NULL)
{
  return new GtkCallerImp5<T, RET, P1, P2, P3, P4, P5>(call, obj);
}

/** creation of a caller, 6 parameters and the gpointer parameter. */
template <class T, typename RET, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6>
GtkCaller *gtk_callback(RET (T::*call)(P1, P2, P3, P4, P5, P6, gpointer),
                        gpointer obj = NULL)
{
  return new GtkCallerImp6<T, RET, P1, P2, P3, P4, P5, P6>(call, obj);
}

/** creation of a caller, 7 parameters and the gpointer parameter. */
template <class T, typename RET, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6, typename P7>
GtkCaller *gtk_callback(RET (T::*call)(P1, P2, P3, P4, P5, P6, P7, gpointer),
                        gpointer obj = NULL)
{
  return new GtkCallerImp7<T, RET, P1, P2, P3, P4, P5, P6, P7>(call, obj);
}

/** creation of a caller, 8 parameters and the gpointer parameter. */
template <class T, typename RET, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6, typename P7, typename P8>
GtkCaller *gtk_callback(RET (T::*call)(P1, P2, P3, P4, P5, P6, P7, P8,
                                       gpointer),
                        gpointer obj = NULL)
{
  return new GtkCallerImp8<T, RET, P1, P2, P3, P4, P5, P6, P7, P8>(call, obj);
}

/** Implement a callback with a caller;

    use caller->callback() to get a callback
    function, and insert the caller pointer into user_data */
inline GtkCaller *gtk_caller_new(const GtkCaller &caller, gpointer client,
                                 gpointer user_data = NULL)
{
  auto cb = caller.clone(client);
  cb->setGPointer(user_data);
  return cb;
}

/** Structure that assembles a widget name, a callback function and
    the widget signal that should trigger the callback function. */
struct GladeCallbackTable
{
  /** Widget name for linking callback */
  const char *widget;
  /** GTK signal name. */
  const char *signal;
  /** Function receiving the callback. Create this one with a
      gtk_callback() function. */
  GtkCaller *func;
  /** User data pointer. */
  gpointer user_data;
};

/** A GUI window directly from a glade interface file. Supply the
    interface file name, and a table of pointers to callback
    functions. As an example:
    @code{.cxx}
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
    @endcode

    Check the signatures for the callback functions in the Gtk4
    documentation. Stick to the signature generated by glade, since
    deviating from that signature may result in all kinds of unclean
    mess.

    The last argument of the callback function, the gpointer argument,
    is normally a pointer to user data that one can add to the signal
    connection in gtk. However, the callback system used by
    GtkGladeWindow already uses *that* gpointer. You can add a new
    value by specifying it in the last column of the
    GladeCallbackTable. This will help you re-use the same callback 
    for different purposes.

    It is also possible to quickly link and load DCO objects to
    elements in your interface. Take the following dco object as an
    example:

    @code{.scheme}
    (Type float)
    (Enum CmdType On Off Schroedinger Invalid)
    (Object TestObject
            (float a (Default 0.1f))
            (CmdType command (Default Off))
            )
    @endcode

    If you now ensure that the widgets holding this data (for the
    float a TextEntry, an Adjustment, a SpinButton or a Range, for the
    enum a DropDown) are properly named, the GtkGladeWindow can:

    - Set the options for the DropDown based on the enum values
    - Set the data from the "a" and "command" members into the interface
    - Convert data from the interface to the "a" and "command" members.

    As an example, for a glade window with a SpinButton named
    "mywidgets_a" and a DropDown named "mywidgets_command", the
    following code should work:

    @code{.cxx}
    // define a mapping between the enum values, and interface strings
    // note that whithout mappings (use NULL), the enum values are used
    // directly in the interface. The mapping must remain valid, use a
    // "static" keyword for that.

    // each enum gets a mapping to label strings
    static const GtkGladeWindow::OptionMapping mapping_command[] = {
      { "On", "Device on" },    // instead of "On", the combo lists "Device On"
      { "Off", "Device off" },  // etc.
                                // option "Schroedinger" is simply shown 
                                // as "Schroedinger"
      { "Invalid", NULL },      // option "Invalid" will not be selectable
      { NULL, NULL }
    };

    // all mappings together, linked to the member name
    static const OptionMappings mappings[] = {
      { "command", mapping_command },
      { NULL, NULL }
    };

    // apply the mappings to the opened window
    // this looks through the mappings, finds "command" there, 
    // then finds "mywidgets_command" in the gui (ID of widget), 
    // and loads the options into the dropdown as per the 
    // mapping_command table.
    mywindow.fillOptions("TestObject", "mywidgets_%s", NULL,
                         mappings, true);

    // set the default values of a TestObject on the interface
    TestObject deflt;
    CommObjectReader reader("TestObject", reinterpret_cast<void*>(&deflt));
    mywindow.setValues(reader, "mywidgets_%s", NULL, true);

    // .... later, after changes, read the values from the interface
    // send over a channel if applicable
    {
      // a DCOWriter is also a CommObjectWriter
      DCOWriter writer(w_mytoken, ts);
      mywindow.getValues(writer, "mywidgets_%s", NULL, true);
    }
    @endcode

    You can also use (fixed-size) arrays with values in the DCO objects,
    and have the interface fill these; the second format string "arrformat",
    describes how these are labeled.

    In your glade gui, ensure that:

    - Each DropDown that you want to use has a text entry

    - Specify column 0 of the associated GListStore for the entry if
      you don't want a mapping (directly use the enum names), column
      1 if you do want a mapping.

    - A DropDown does not need a GListStore in the gui file; if none
      is found, one will be automatically generated.

    - The widgets you want to connect to a DCO object need an ID that
      matches the DCO object member you want to link, e.g., ID
      "myui_speed" will link to the DCO member "speed", if you specify
      "myui_%s" as the format.

    - When linking to an array in a DCO object, use a number in the ID,
      e.g., array format "myui_%s[%d]" enables you to link widgets with
      ID's "myui_button[0]", "myui_button[1]", etc., to elements in an
      array in the DCO object named "button"

 */
class GtkGladeWindow
{
  /** A flag that avoids multiple initialization of glade. */
  static bool initialised_glade;

  /** A flag that avoids multiple initialization of gtkmm. */
  static bool initialised_gtkmm;

  /** The main window, if present. */
  GtkWidget *window;

  /** Builder */
  GtkBuilder *builder;

  /** X coordinate of requested window position, -1 if default */
  int offset_x;

  /** Y coordinate of requested window position, -1 if default */
  int offset_y;

  /** width of requested, 0 if default */
  int size_x;

  /** heigth of requested, 0 if default */
  int size_y;

  /** A map of already initialized C++ widget objects, gtkmm interface */
  std::map<std::string, Gtk::Widget *> widgets;

  /** Helper, set a double value on a widget */
  bool _setValue(const char *wname, double value, bool warn);

  /** Helper, set a string value on a widget */
  bool _setValue(const char *wname, const char *value, bool warn);

  /** Helper, set a state on a widget */
  bool _setValue(const char *wname, bool value, bool warn);

  /** Helper, set any value on a widget */
  bool _setValue(const char *wname, const CommObjectReader &cor, unsigned im,
                 boost::any &b, bool warn);

  /** Helper, get a state from a widget */
  template <class T>
  bool __getValue(const char *wname, boost::any &value, bool warn);

  /** Helper, get any value from a widget */
  bool _getValue(const char *wname, const CommObjectWriter& cow, unsigned ii,
                 boost::any &b, bool warn);

public:
  /** Constructor. */
  GtkGladeWindow();

  /** Destructor. Also closes the window if not already closed. */
  ~GtkGladeWindow();

  /** Initialization of the glade window.

      Loads the interface code from the file and creates the main
      widget. Connects callbacks given in the table, and optionally
      connects callback signals to symbols found in the application's
      symbol table.

      @param file        Name of the glade interface file
      @param mainwidget  Top-level widget (normally a window) in that
                         file that will be opened.
      @param client      Pointer to the class that will be receiving
                         the callbacks.
      @param table       Table linking widget, signal, callback
                         function and optionally the pointer argument
                         to the callback function.
      @param connect_signals Obsolete for the gtk4 version.
      @param warn        Warn when widgets in the callback table are not
                         found in the interface.
      @returns           true if all OK. */
  bool readGladeFile(const char *file, const char *mainwidget,
                     gpointer client = NULL,
                     const GladeCallbackTable *table =
                       reinterpret_cast<GladeCallbackTable *>(NULL),
                     bool connect_signals = false, bool warn = true);

  /** Connect callbacks to widgets. You can repeatedly use this
      function, adding more callbacks to the widgets.
      @param client      Pointer to the class that will be receiving
                         the callbacks.
      @param table       Table linking widget, signal, callback
                         function and optionally the pointer argument
                         to the callback function.
      @param warn        Warn when widgets in the callback table are not
                         found in the interface. */
  void connectCallbacks(gpointer client, const GladeCallbackTable *table,
                        bool warn = true);

  /** Connect callbacks to widgets. Callbacks will be added as last.
      You can repeatedly use this
      function, adding more callbacks to the widgets.
      @param client      Pointer to the class that will be receiving
                         the callbacks.
      @param table       Table linking widget, signal, callback
                         function and optionally the pointer argument
                         to the callback function.
      @param warn        Warn when widgets in the callback table are not
                         found in the interface. */
  void connectCallbacksAfter(gpointer client, const GladeCallbackTable *table,
                             bool warn = true);

  /** Access the widgets in this interface.

      Most objects in the interface will be widgets; note that for
      GtkListStore and GtkTreeStore objects you need getObject().

      @param wname       Widget name.
      @returns           A widget object, NULL when the object is not found.
  */
  GtkWidget *operator[](const char *wname) const;

  /** Access anything in the interface file as objects

      @param name        Object name.
      @returns           A GObject pointer, NULL when the object is not found.
  */
  GObject *getObject(const char *name) const;

  /** Open the window

      By default, this uses the main window/widget.

      @param widget  Optional, widget name to show other widget or window
  */
  void show(const char *widget = NULL);

  /** Close the window.

      By default, this uses the main window/widget.

      @param widget  Optional, widget name to hide other widget or
                     window.
  */
  void hide(const char *widget = NULL);

  /** Callback for placing the window */
  void placeWindow(GtkWidget *w, gpointer user_data);

  /** Struct for mapping enum name to representation string */
  struct OptionMapping
  {
    /** C++ name of an enumerated value */
    const char *ename;
    /** String by which this should be shown in the interface */
    const char *representation;
  };

  /** Struct for describing mappings */
  struct OptionMappings
  {
    /** C++ name of the dco member variable */
    const char *dcomember;
    /** Ponter to a NULL-value terminated list of mappings for the
        enumerated value to representation */
    const OptionMapping *mapping;
  };

private:
  /** Helper for option insertion */
  bool _fillOptions(const char *wname, ElementWriter &writer,
                    ElementReader &reader, const OptionMapping *mapping,
                    bool warn);

  /** Helper for enum to radio mapping */
  bool _getEnumFromRadios(const char *gtkid, const CommObjectWriter &dco,
                          unsigned im, boost::any &b, bool warn);

  /** Helper for enum to radio mapping */
  bool _setRadiosFromEnum(const char *gtkid, const CommObjectReader &dco,
                          unsigned im, boost::any &b, bool warn);

public:
  /** Use the enum items in a DCO object to fill DropDown tree models
      in the interface

      @param dcoclass  Object class
      @param format    Format string, to be written with sprintf,
                       use "%s" to insert the element name, e.g.,
                       "mywidgets_%s"
      @param arrformat Format string to be used when connecting to
                       an array element, e.g., "mywidgets_%s_%02d"
      @param mapping   Optional mapping table, defining sets of
                       member name + enum string, to representation,
                       NULL-terminated.
      @param warn      If true, provide warnings when gui elements
                       cannot be found, or have the wrong type.
  */
  bool fillOptions(const char *dcoclass, const char *format,
                   const char *arrformat = NULL,
                   const OptionMappings *mapping = NULL, bool warn = false);

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
  unsigned setValues(CommObjectReader &dco, const char *format,
                     const char *arrformat = NULL, bool warn = false);

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
  unsigned getValues(CommObjectWriter &dco, const char *format,
                     const char *arrformat = NULL, bool warn = false);

  /** Initialize a text dropdown with a given list or array of values.

      @param name      Name of the dropdown box.
      @param values    Iterable (list, vector, etc.) of std::string
                       (or string-like) objects, these need a "c_str()" method.
   */
  template <typename T>
  bool loadDropDownText(const char *name, const T &values);

  /** \brief Obtain gtk widget with name 'name' as a C++ object
    *
    * Usage example:
    * \code
        Gtk::Button* b = NULL;
        myGladeWindow.getWidget("myButton", b);
    * \endcode
    */
  template <typename T> T *getWidget(const std::string &name, T *&w);

  /** \brief Obtain gtk widget with name 'name' as a C++ object using
      a derived class
    *
    * Usage example:
    * \code
      MyDerivedButton* b = NULL;
      myGladeWindow.getWidgetDerived("myButton", b);
    * \endcode
    */
  template <typename T> T *getWidgetDerived(const std::string &name, T *&w);

  /** \brief Obtain gtk widget with name 'name' as a C++ object
    *
    * Usage example:
    * \code
      Gtk::Widget* b = myGladeWindow.getWidget("myButton");
    * \endcode
    */
  Gtk::Widget *getWidget(const std::string &name);

  /// Init function for gtkmm
  void initGtkMM();

  /** Change position and size of the window.
      @param  p    Vector with offset and size elements, p[0] offset
                   x. p[1] offset y, if either < 0, position hints are
                   ignored. p[2] width, p[3] height, if either <= 0,
                   size hints are ignored. Vector must have 2 or 4 elements. */
  inline bool setWindow(const std::vector<int> &p)
  {
    if (p.size() == 2 || p.size() == 4) {
      offset_x = p[0];
      offset_y = p[1];
    }
    if (p.size() == 4) {
      size_x = p[2];
      size_y = p[3];
    }
    return p.size() == 2 || p.size() == 4;
  }
};

template <typename T>
T *GtkGladeWindow::getWidget(const std::string &name, T *&w)
{
  w = dynamic_cast<T *>(getWidget(name));
  return w;
}

template <typename T>
T *GtkGladeWindow::getWidgetDerived(const std::string &name, T *&w)
{
  if (!initialised_gtkmm)
    initGtkMM();

  std::map<std::string, Gtk::Widget *>::iterator it = widgets.find(name);
  if (it != widgets.end()) {
    return dynamic_cast<T *>(it->second);
  }
  else {
    if (GtkWidget *cw =
          GTK_WIDGET(gtk_builder_get_object(builder, name.c_str()))) {
      w = new T(cw);
      widgets[name] = w;
      return w;
    }
    else {
      throw(GladeException("Widget \"" + name + "\" does not exist!"));
    }
  }
}
DUECA_NS_END;

#include <dueca/debug.h>

DUECA_NS_START;

template <typename T>
bool GtkGladeWindow::loadDropDownText(const char *name, const T &values)
{
  GObject *o = getObject(name);
  if (o == NULL) {
    /* DUECA graphics.

       You are trying to load options into a GTK widget, but the
       widget cannot be found. Check the widget name and ui file.
     */
    W_XTR(
      "GtkGladeWindow::loadDropDownText: Could not find gtk object with id \""
      << name << "\"");
    return false;
  }

  if (GTK_IS_DROP_DOWN(o)) {

    // check whether a model or the right kind of model is set
    auto model = G_LIST_STORE(gtk_drop_down_get_model(GTK_DROP_DOWN(o)));
    if (!model) {
      model = g_list_store_new(gtk_string_object_get_type());
      gtk_drop_down_set_model(GTK_DROP_DOWN(o), G_LIST_MODEL(model));
    }

    // reader and writer are coupled, run through all options
    for (const auto &s : values) {
      g_list_store_append(model, gtk_string_object_new(s.c_str()));
    }
    return true;
  }

  /* DUECA graphics.

     Cannot load text options into the given widget, it is not a
     combo box or drop down
  */
  W_XTR("GtkGladeWindow::loadDropDownText: Cannot fill options, object not a "
        "DropDown \""
        << name << '"');
  return false;
}

DUECA_NS_END
#include <dueca/undebug.h>
#endif
