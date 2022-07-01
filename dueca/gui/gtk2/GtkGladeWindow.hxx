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

#include <dueca_ns.h>
#include <glade/glade.h>
#include <glade/glade-xml.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include "GtkCaller.hxx"
#include "GladeException.hxx"

// Forward declaration
namespace Gtk {
  class Widget;
}

/** \file gtk2/GtkGladeWindow.hxx
    Gtk GUI facilities. */

DUECA_NS_START

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

  /** The window. */
  GtkWidget* window;

  /** The xml tree */
  GladeXML* xmltree;

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

  /** A map of already initialized C++ widget objects */
  std::map<std::string, Gtk::Widget*> widgets;

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
      \param main_only   If set to true, only loads 'mainwidget' and
                         its children. Otherwise it loads all windows
                         and widgets from the glade file.
      \returns           true if all OK. */
  bool readGladeFile(const char* file,
                     const char* mainwidget,
                     gpointer client = NULL,
                     const GladeCallbackTable *table =
                     reinterpret_cast<GladeCallbackTable*>(NULL), bool main_only = true);

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

  /** Access the widgets in this interface.
      \param wname       Widget name. */
  GtkWidget* operator [] (const char* wname);

  /** Open the window */
  void show();

  /** Close the window. */
  void hide();

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

  /** \brief Obtain gtk widget with name 'name' as a C++ object using a derived class
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
  inline bool setWindow(const std::vector<int>& p)
  { if (p.size() == 2 || p.size() == 4) {
      offset_x = p[0]; offset_y = p[1];
      if (p.size() == 4) { size_x = p[2]; size_y = p[3]; }
      return true;
    }
    return false;
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
    if (GtkWidget* cw = glade_xml_get_widget(xmltree, name.c_str())) {
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
