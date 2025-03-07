/* ------------------------------------------------------------------   */
/*      item            : GtkGladeWindow.cxx
        made by         : Rene' van Paassen
        date            : 051017
        category        : body file
        description     :
        changes         : 051017 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "gtk/gtk.h"
#define GtkGladeWindow_cxx

#include <dueca-conf.h>
#include "GtkGladeWindow.hxx"
#include <dueca/CommObjectReader.hxx>
#include <dueca/CommObjectWriter.hxx>
#include <dueca/CommObjectElementReader.hxx>
#include <dueca/CommObjectElementWriter.hxx>
#include <dueca/DataClassRegistry.hxx>
#include <dueca/CommObjectMemberAccess.hxx>
#include <dueca/DataSetConverter.hxx>
#include "gdk/gdk.h"
#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#endif
#define E_CNF
#define W_CNF
#include "debug.h"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

bool GtkGladeWindow::initialised_glade = false;
bool GtkGladeWindow::initialised_gtkmm = false;

// TODO: all combobox (obsolete) offer also drop_down
// variant

GtkGladeWindow::GtkGladeWindow() :
  window(NULL),
  builder(NULL),
  offset_x(-1),
  offset_y(-1),
  size_x(0),
  size_y(0)
{
  //
}

GtkGladeWindow::~GtkGladeWindow()
{
  hide();
  g_object_unref(builder);
}

void GtkGladeWindow::placeWindow(GtkWidget *win, gpointer user_data)
{
  auto gdk_display_id = gdk_display_get_default();
  if (GDK_IS_X11_DISPLAY(gdk_display_id)) {
    auto surf = GDK_SURFACE(gtk_native_get_surface(GTK_NATIVE(win)));
    if (surf) {
      auto xw = GDK_SURFACE_XID(surf);
      auto xd = GDK_SURFACE_XDISPLAY(surf);
      if (xd) {
        XMoveWindow(xd, xw, offset_x, offset_y);
      }
    }
  }
  else if (GDK_IS_WAYLAND_DISPLAY(gdk_display_id)) {
    /* DUECA extra.

       Under wayland, it is (currently) not possible to request a window
       position
    */
    W_XTR("Cannot influence window position on wayland");
  }
}

static void GtkGladeWindow_placeWindow(GtkWidget *win, gpointer _self)
{
  reinterpret_cast<GtkGladeWindow *>(_self)->placeWindow(win, NULL);
}

bool GtkGladeWindow::readGladeFile(const char *file, const char *mainwidget,
                                   gpointer client,
                                   const GladeCallbackTable *table,
                                   bool connect_signals, bool warn)
{
  if (builder == NULL)
    builder = gtk_builder_new();

  gtk_builder_add_from_file(builder, file, NULL);

  // get the main widget, if this is the first file being read
  if (!window) {
    if (mainwidget) {
      window = GTK_WIDGET(gtk_builder_get_object(builder, mainwidget));
      if (!window) {
        /* DUECA graphics.

           Cannot find the main widget in the glade builder windowing
           file. Please check the file and main widget name.
        */
        E_CNF("Cannot find main widget " << mainwidget << " in " << file);
        return false;
      }
    }
    else if (mainwidget) {
      /* DUECA graphics.
      
         You are trying to load a second ui file, and select a main widget, 
         but the main window/widget is already set. Ignoring this mainwidget
         name.
      */
      W_CNF("Second window specified for Gtk builder.");
    }
  }

  // position requested?
  if (mainwidget && offset_x >= 0 && offset_y >= 0) {
    g_signal_connect(window, "realize", G_CALLBACK(GtkGladeWindow_placeWindow),
                     this);
  }

  // size requested?
  if (mainwidget && size_x > 0 && size_y > 0) {
    gtk_widget_set_size_request(window, size_x, size_y);
  }

  // connect all required buttons.
  if (client && table) {
    connectCallbacks(client, table, warn);
  }

  // done
  return true;
}

void GtkGladeWindow::connectCallbacks(gpointer client,
                                      const GladeCallbackTable *table,
                                      bool warn)
{
  const GladeCallbackTable *cbl = table;
  while (cbl->widget) {

    // lookup the widget and link the function
    GObject *b = gtk_builder_get_object(builder, cbl->widget);
    if (b) {
      GtkCaller *caller = cbl->func->clone(client);
      caller->setGPointer(cbl->user_data);
      // link the callback function
      g_signal_connect(b, cbl->signal, caller->callback(), caller);
    }

    else if (warn) {
      /* DUECA graphics.

           Cannot find a widget in the gtk-builder model, when trying
           to connect the callbacks.
        */
      W_CNF("GtkGladeWindow::connectCallbacks: Cannot find widget "
            << cbl->widget);
    }

    // to next cbl.
    cbl++;
  }
}

void GtkGladeWindow::connectCallbacksAfter(gpointer client,
                                           const GladeCallbackTable *table,
                                           bool warn)
{
  const GladeCallbackTable *cbl = table;
  while (cbl->widget) {

    // lookup the widget and link the function
    GObject *b = gtk_builder_get_object(builder, cbl->widget);
    if (b) {
      GtkCaller *caller = cbl->func->clone(client);
      caller->setGPointer(cbl->user_data);
      // link the callback function
      g_signal_connect_after(b, cbl->signal, caller->callback(), caller);
    }

    else if (warn) {
      /* DUECA graphics.

           Cannot find a widget in the gtk-builder model, when trying
           to connect the callbacks.
        */
      W_CNF("GtkGladeWindow::connectCallbacksAfter: Cannot find widget "
            << cbl->widget);
    }

    // to next cbl.
    cbl++;
  }
}

GtkWidget *GtkGladeWindow::operator[](const char *wname) const
{
  if (builder)
    return GTK_WIDGET(gtk_builder_get_object(builder, wname));
  return NULL;
}

GObject *GtkGladeWindow::getObject(const char *wname) const
{
  if (builder)
    return gtk_builder_get_object(builder, wname);
  return NULL;
}

void GtkGladeWindow::show(const char *widget)
{
  if (widget) {
    gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(builder, widget)),
                           TRUE);
  }
  else {
    gtk_widget_set_visible(window, TRUE);
  }
}

void GtkGladeWindow::hide(const char *widget)
{
  if (widget) {
    gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(builder, widget)),
                           FALSE);
  }
  else {
    gtk_widget_set_visible(window, FALSE);
  }
}

bool GtkGladeWindow::_setValue(const char *wname, double value, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      /* DUECA graphics.

           When trying to set a value in the interface, the widget name
           corresponding to the DCO member name was not found. This may
           be a typo in your ui definition, or incidental.
        */
      W_XTR("GtkGladeWindow::setValue: Could not find gtk object with id \""
            << wname << "\"");
    }
    return false;
  }

  // try 1, adjustment
  if (GTK_IS_ADJUSTMENT(o)) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(o), value);
    return true;
  }

  // try 2, range
  if (GTK_IS_RANGE(o)) {
    gtk_range_set_value(GTK_RANGE(o), value);
    return true;
  }

  // try 3, spinbutton
  if (GTK_IS_SPIN_BUTTON(o)) {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(o), value);
    return true;
  }

  // try 3, entry, after spinbutton, bc that inherits from entry
  if (GTK_IS_ENTRY(o)) {
    std::string svalue = boost::lexical_cast<std::string>(value);
    gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(o)),
                              svalue.c_str(), svalue.size());
    return true;
  }

  // also try for a dropdown
  if (GTK_IS_DROP_DOWN(o)) {
    auto _model = gtk_drop_down_get_model(GTK_DROP_DOWN(o));
    if (GTK_IS_STRING_LIST(_model)) {
      auto model = GTK_STRING_LIST(_model);
      for (auto n = g_list_model_get_n_items(G_LIST_MODEL(model)); n--;) {
        DEB("Testing " << gtk_string_list_get_string(model, n));
        if (boost::lexical_cast<double>(gtk_string_list_get_string(model, n)) ==
            value) {
          gtk_drop_down_set_selected(GTK_DROP_DOWN(o), n);
          return true;
        }
      }
    }
  }

  if (warn) {
    /* DUECA graphics.

         The widget is not compatible with float or double values from
         the associated DCO object.
       */
    W_XTR("GtkGladeWindow::setValue: Setting double/float for gtk object \""
          << wname << "\" not implemented");
  }
  return false;
}

static const char *
_representationInMap(const GtkGladeWindow::OptionMapping *mapping,
                     const char *key, bool warn)
{
  for (auto m = mapping; m->ename != NULL; m++) {
    if (!strcmp(m->ename, key)) {
      if (m->representation != NULL) {
        return m->representation;
      }
      else {
        return key;
      }
    }
  }
  if (warn) {
    /* DUECA graphics.

         In the given key is missing from the option string mapping for
         selecting an Enum with a ComboBox or DropDown. Check the mapping
         against the DCO definition for the enum.
      */
    W_XTR("GtkGladeWindow::fillOptions: Key \""
          << key << "\" not given in options mapping");
  }
  return key;
}

static const char *_keyFromMap(const GtkGladeWindow::OptionMapping *mapping,
                               unsigned idx)
{
  auto m = mapping;
  for (auto i = idx; i--;) {
    if (m && m->ename) {
      if (idx) {
        m++;
      }
      else {
        return m->ename;
      }
    }
    else {
      return NULL;
    }
  }
  return NULL;
}

static int _indexInMap(const GtkGladeWindow::OptionMapping *mapping,
                       const char *key)
{
  int idx = 0;
  auto m = mapping;
  for (; m->ename && strcmp(m->ename, key); m++) {
    idx++;
  }
  if (m->ename)
    return idx;
  return -1;
}

bool GtkGladeWindow::_fillOptions(const char *wname, ElementWriter &writer,
                                  ElementReader &reader,
                                  const OptionMapping *mapping, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      /* DUECA graphics.

           Cannot find the given object; check whether it is in the interface,
           or check for spelling errors.
        */
      W_XTR("GtkGladeWindow::fillOptions: Could not find gtk object with id \""
            << wname << "\"");
    }
    return false;
  }

  if (GTK_IS_DROP_DOWN(o)) {

    // check whether a model or the right kind of model is set
    auto model = gtk_string_list_new(NULL);

    // reader and writer are coupled, run through all options
    writer.setFirstValue();
    do {
      std::string value;
      reader.peek(value);
      if (mapping) {
        value = _representationInMap(mapping, value.c_str(), warn);
      }
      gtk_string_list_append(model, value.c_str());
    }
    while (writer.setNextValue());

    gtk_drop_down_set_model(GTK_DROP_DOWN(o), G_LIST_MODEL(model));
    return true;
  }

  // when using a mapping, set a pointer in a property
  if (mapping) {
    g_object_set_data(o, "d_mapping", gpointer(mapping));
  }

  // when here, neither combobox nor dropdown to fill
  if (warn) {
    /* DUECA graphics.

         Cannot feed options to the given object; check whether it is
         a GtkComboBox or GtkDropDown.
      */
    W_XTR("GtkGladeWindow::fillOptions: Cannot fill options, object not a "
          "ComboBox or DropDown\""
          << wname << '"');
  }
  return false;
}

bool GtkGladeWindow::_setValue(const char *wname, const char *value, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      /* DUECA graphics.

           When trying to set a value in the interface, the widget name
           corresponding to the DCO member name was not found. This may
           be a typo in your ui definition, or incidental.
        */
      W_XTR("GtkGladeWindow::setValue: Could not find gtk object with id \""
            << wname << "\"");
    }
    return false;
  }

  if (GTK_IS_DROP_DOWN(o)) {
    auto *mapping =
      reinterpret_cast<OptionMapping *>(g_object_get_data(o, "d_mapping"));
    if (mapping) {
      auto res = _indexInMap(mapping, value);
      if (res >= 0) {
        gtk_drop_down_set_selected(GTK_DROP_DOWN(o), res);
      }
      else {
        return false;
      }
    }
    else {
      auto model = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(o)));
      guint pos = 0;
      auto item = gtk_string_list_get_string(model, pos);
      while (item) {
        if (strcmp(item, value) == 0) {
          gtk_drop_down_set_selected(GTK_DROP_DOWN(o), pos);
          return true;
        }
        item = gtk_string_list_get_string(model, ++pos);
      }
      return false;
    }
  }

  if (GTK_IS_ENTRY(o)) {
    GtkEntry *e = GTK_ENTRY(o);
    if (e != NULL) {
      gtk_entry_buffer_set_text(gtk_entry_get_buffer(e), value, strlen(value));
      return true;
    }
  }

  if (warn) {
    /* DUECA graphics.

         Trying to set a text for a widget, but the widget is neither
         a combobox, a drop down, nor a text entry.
      */
    W_XTR("GtkGladeWindow::setValue: Setting text for gtk object \""
          << wname << "\" not implemented");
  }
  return false;
}

bool GtkGladeWindow::_setValue(const char *wname, bool value, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      /* DUECA graphics.

           When trying to set a value in the interface, the widget name
           corresponding to the DCO member name was not found. This may
           be a typo in your ui definition, or incidental.
        */
      W_XTR("GtkGladeWindow::setValue: Could not find gtk object with id \""
            << wname << "\"");
    }
    return false;
  }

  if (GTK_IS_TOGGLE_BUTTON(o)) {
    GtkToggleButton *t = GTK_TOGGLE_BUTTON(o);
    gtk_toggle_button_set_active(t, value ? TRUE : FALSE);
    return true;
  }
  else if (GTK_IS_CHECK_BUTTON(o)) {
    auto t = GTK_CHECK_BUTTON(o);
    gtk_check_button_set_active(t, value ? TRUE : FALSE);
    return true;
  }

  if (warn) {
    /* DUECA graphics.

         Trying to set an on-off state for a widget, but the widget is not a
         toggle button.
      */
    W_XTR("GtkGladeWindow::setValue: Setting state for gtk object \""
          << wname << "\" not implemented");
  }
  return false;
}

bool GtkGladeWindow::_setValue(const char *wname, const CommObjectReader &cor,
                               unsigned im, boost::any &b, bool warn)
{
  if (cor.getMemberAccessor(im).isEnum() &&
      _setRadiosFromEnum(wname, cor, im, b, false)) {
    return true;
  }
  else if (b.type() == typeid(double)) {
    return _setValue(wname, boost::any_cast<double>(b), warn);
  }
  else if (b.type() == typeid(float)) {
    return _setValue(wname, boost::any_cast<float>(b), warn);
  }
  else if (b.type() == typeid(int8_t)) {
    return _setValue(wname, double(boost::any_cast<int8_t>(b)), warn);
  }
  else if (b.type() == typeid(int16_t)) {
    return _setValue(wname, double(boost::any_cast<int16_t>(b)), warn);
  }
  else if (b.type() == typeid(int32_t)) {
    return _setValue(wname, double(boost::any_cast<int32_t>(b)), warn);
  }
  else if (b.type() == typeid(int64_t)) {
    return _setValue(wname, double(boost::any_cast<int64_t>(b)), warn);
  }
  else if (b.type() == typeid(uint8_t)) {
    return _setValue(wname, double(boost::any_cast<uint8_t>(b)), warn);
  }
  else if (b.type() == typeid(uint16_t)) {
    return _setValue(wname, double(boost::any_cast<uint16_t>(b)), warn);
  }
  else if (b.type() == typeid(uint32_t)) {
    return _setValue(wname, double(boost::any_cast<uint32_t>(b)), warn);
  }
  else if (b.type() == typeid(uint64_t)) {
    return _setValue(wname, double(boost::any_cast<uint64_t>(b)), warn);
  }
  else if (b.type() == typeid(bool)) {
    return _setValue(wname, boost::any_cast<bool>(b), warn);
  }
  else if (b.type() == typeid(std::string)) {
    return _setValue(wname, boost::any_cast<std::string>(b).c_str(), warn);
  }
  try {
    // try this with enum's, strings, any....
    return _setValue(wname, boost::any_cast<std::string>(b).c_str(), warn);
  }
  catch (const std::exception &) {
    //
  }
  if (warn) {
    /* DUECA graphics.

       Could not interpret the data of a DCO member */
    W_XTR("GtkGladeWindow::setValue: could not interpret type of member "
          << cor.getMemberName(im));
  }
  return false;
}

template <class T>
bool GtkGladeWindow::__getValue(const char *wname, boost::any &b, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      /* DUECA graphics.

         When trying to get a value from the interface, the widget name
         corresponding to the DCO member name was not found. This may
         be a typo in your ui definition, or incidental.
      */
      W_XTR("GtkGladeWindow::getValue: Could not find gtk object with id \""
            << wname << "\"");
    }
    return false;
  }

  // try 1, adjustment
  if (GTK_IS_ADJUSTMENT(o)) {
    b = T(gtk_adjustment_get_value(GTK_ADJUSTMENT(o)));
    return true;
  }

  // try 2, range
  if (GTK_IS_RANGE(o)) {
    b = T(gtk_range_get_value(GTK_RANGE(o)));
    return true;
  }

  // try 3, spinbutton
  if (GTK_IS_SPIN_BUTTON(o)) {
    b = T(gtk_spin_button_get_value(GTK_SPIN_BUTTON(o)));
    return true;
  }

  // try 3, entry, after spinbutton, bc that inherits from entry
  if (GTK_IS_ENTRY(o)) {
    b = boost::lexical_cast<T>(
      gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(o))));
    return true;
  }

  if (GTK_IS_DROP_DOWN(o)) {
    auto _model = gtk_drop_down_get_model(GTK_DROP_DOWN(o));
    if (GTK_IS_STRING_LIST(_model)) {
      auto model = GTK_STRING_LIST(_model);
      auto sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(o));
      try {
        b = boost::lexical_cast<T>(gtk_string_list_get_string(model, sel));
        return true;
      }
      catch (const std::exception &e) {
        // not readable
      }
    }
  }

  if (warn) {
    /* DUECA graphics.

       Trying to get a numeric value from a widget, but getting a
       numeric value from this widget type is not supported.
    */
    W_XTR("GtkGladeWindow::getValue: Getting double/float for gtk object \""
          << wname << "\" not implemented");
  }
  return false;
}

template <>
bool GtkGladeWindow::__getValue<bool>(const char *wname, boost::any &b,
                                      bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      /* DUECA graphics.

           When trying to get a value from the interface, the widget name
           corresponding to the DCO member name was not found. This may
           be a typo in your ui definition, or incidental.
        */
      W_XTR("GtkGladeWindow::getValue: Could not find gtk object with id \""
            << wname << "\"");
    }
    return false;
  }

  if (GTK_IS_TOGGLE_BUTTON(o)) {
    b = bool(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(o)));
    return true;
  }
  else if (GTK_IS_CHECK_BUTTON(o)) {
    b = bool(gtk_check_button_get_active(GTK_CHECK_BUTTON(o)));
    return true;
  }

  if (warn) {
    /* DUECA graphics.

         Getting a boolean value from this widget type is not supported.
       */
    W_XTR("GtkGladeWindow::getValue: Getting state for gtk object \""
          << wname << "\" not implemented");
  }
  return false;
}

template <>
bool GtkGladeWindow::__getValue<std::string>(const char *wname, boost::any &b,
                                             bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      /* DUECA graphics.

           When trying to get a value from the interface, the widget name
           corresponding to the DCO member name was not found. This may
           be a typo in your ui definition, or incidental.
        */
      W_XTR("GtkGladeWindow::getValue: Could not find gtk object with id \""
            << wname << "\"");
    }
    return false;
  }

  if (GTK_IS_DROP_DOWN(o)) {
    auto item = gtk_drop_down_get_selected(GTK_DROP_DOWN(o));
    auto *mapping =
      reinterpret_cast<OptionMapping *>(g_object_get_data(o, "d_mapping"));
    const char *res = NULL;
    if (mapping) {
      res = _keyFromMap(mapping, item);
    }
    else {
      auto slist = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(o)));
      res = gtk_string_list_get_string(slist, item);
    }
    if (res) {
      b = std::string(res);
      return true;
    }
    else if (warn) {
      /* DUECA graphics.

           Attempting to get an active entry from a drop down, but none
           is active. Maybe pre-select an entry.
         */
      W_XTR("GtkGladeWindow::getValue, no active entry in drop down \"" << wname
                                                                        << '"');
    }
    return false;
  }

  if (GTK_IS_ENTRY(o)) {
    GtkEntry *e = GTK_ENTRY(o);
    b = std::string(gtk_entry_buffer_get_text(gtk_entry_get_buffer(e)));
    return true;
  }

  if (warn) {
    /* DUECA graphics.

         Getting a text value from this widget type is not supported.
       */
    W_XTR("GtkGladeWindow::getValue: Getting text for gtk object \""
          << wname << "\" not implemented");
  }
  return false;
}

bool GtkGladeWindow::_getValue(const char *wname, const CommObjectWriter &cor,
                               unsigned im, boost::any &value, bool warn)
{
  if (cor.getMemberAccessor(im).isEnum() &&
      _getEnumFromRadios(wname, cor, im, value, warn)) {
    return true;
  }
  if (!strcmp(cor.getMemberClass(im), "double")) {
    return __getValue<double>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "float")) {
    return __getValue<float>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "int8_t")) {
    return __getValue<int8_t>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "int16_t")) {
    return __getValue<int16_t>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "int32_t")) {
    return __getValue<int32_t>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "int64_t")) {
    return __getValue<int64_t>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "uint8_t")) {
    return __getValue<uint8_t>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "uint16_t")) {
    return __getValue<uint16_t>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "uint32_t")) {
    return __getValue<uint32_t>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "uint64_t")) {
    return __getValue<uint64_t>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "bool")) {
    return __getValue<bool>(wname, value, warn);
  }
  if (!strcmp(cor.getMemberClass(im), "std::string")) {
    return __getValue<std::string>(wname, value, warn);
  }
  try {
    return __getValue<std::string>(wname, value, warn);
  }
  catch (const std::exception &) {
    //
  }
  if (warn) {
    /* DUECA graphics.

         Could not read data from this DCO member.
      */
    W_XTR("GtkGladeWindow::getValue: Could not interpret type of member \""
          << cor.getMemberName(im) << "\" with class \"" << cor.getClassname()
          << '"');
  }
  return false;
}

unsigned GtkGladeWindow::setValues(CommObjectReader &dco, const char *format,
                                   const char *arrformat, bool warn)
{
  unsigned nset = 0;
  char gtkid[128];
  for (size_t ii = dco.getNumMembers(); ii--;) {
    if (dco.getMemberArity(ii) == Single) {
      snprintf(gtkid, sizeof(gtkid), format, dco.getMemberName(ii));
      boost::any b;
      dco[ii].read(b);
      if (_setValue(gtkid, dco, ii, b, warn)) {
        nset++;
      }
    }
    else if (dco.getMemberArity(ii) == Iterable ||
             dco.getMemberArity(ii) == FixedIterable) {
      if (arrformat != NULL) {
        auto ereader = dco[ii];
        unsigned idx = 0;
        while (!ereader.isEnd()) {
          snprintf(gtkid, sizeof(gtkid), arrformat, dco.getMemberName(ii), idx);
          boost::any b;
          ereader.read(b);
          if (_setValue(gtkid, dco, ii, b, warn)) {
            nset++;
          }
        }
      }
      else {
        /* DUECA graphics.

             You have an array member in the DCO object you try to
             connect to a gtk window, but have not supplied an array
             format string.
          */
        W_XTR("GtkGladeWindow::setValues: No format specified for array member "
              << dco.getMemberName(ii));
      }
    }
    else {
      /* DUECA graphics.

           This member class (mapping, variable size array or nested) cannot
           be used in connecting to a gtk interface.
        */
      W_XTR("GtkGladeWindow::setValues: Could not interpret organisation of "
            "member "
            << dco.getMemberName(ii));
    }
  }
  return nset;
}

bool GtkGladeWindow::_getEnumFromRadios(const char *gtkid,
                                        const CommObjectWriter &dco,
                                        unsigned im, boost::any &b, bool warn)
{
  auto converter = DataClassRegistry::single().getConverter(dco.getClassname());
  void *object = converter->clone(NULL);

  // reader and writer are used to find enum names
  auto eltreader = dco.getMemberAccessor(im).getReader(object);
  auto eltwriter = dco.getMemberAccessor(im).getWriter(object);

  eltwriter.setFirstValue();
  do {
    std::string value;
    eltreader.peek(value);
    auto wname = boost::str(boost::format("%s-%s") % gtkid % value);
    auto w = getObject(wname.c_str());
    if (w && GTK_IS_CHECK_BUTTON(w) &&
        gtk_check_button_get_active(GTK_CHECK_BUTTON(w))) {
      b = value;
      return true;
    }
  }
  while (eltwriter.setNextValue());

  if (warn) {
    /* DUECA graphics.

       Could not find radio buttons (=linked checkbuttons) with naming to match
       a given enum. The base name must match the given dco object name, the
       enum value must be coded after a colon. Example:
       "prefix_dcomembername:One", "prefix_dcomembername:Two", if One, Two are
       the enum values, etc.
    */
    W_XTR("GtkGladeWindow::getValues, no match for radio button to enum");
  }
  return false;
}

bool GtkGladeWindow::_setRadiosFromEnum(const char *gtkid,
                                        const CommObjectReader &dco,
                                        unsigned im, boost::any &b, bool warn)
{
  auto converter = DataClassRegistry::single().getConverter(dco.getClassname());
  void *object = converter->clone(NULL);

  // reader and writer are used to find enum names
  auto eltreader = dco.getMemberAccessor(im).getReader(object);
  auto eltwriter = dco.getMemberAccessor(im).getWriter(object);

  eltwriter.setFirstValue();
  do {
    std::string value;
    eltreader.peek(value);
    auto wname = boost::str(boost::format("%s-%s") % gtkid % value);
    auto w = getObject(wname.c_str());
    if (w && GTK_IS_CHECK_BUTTON(w) &&
        boost::any_cast<std::string>(b) == value) {
      gtk_check_button_set_active(GTK_CHECK_BUTTON(w), TRUE);
      return true;
    }
  }
  while (eltwriter.setNextValue());

  if (warn) {
    /* DUECA graphics.

       Could not find radio buttons (=linked checkbuttons) with naming to match
       a given enum. The base name must match the given dco object name, the
       enum value must be coded after a colon. Example:
       "prefix_dcomembername:One", "prefix_dcomembername:Two", if One, Two are
       the enum values, etc.
    */
    W_XTR("GtkGladeWindow::getValues, no match for enum to radio button");
  }
  return false;
}

unsigned GtkGladeWindow::getValues(CommObjectWriter &dco, const char *format,
                                   const char *arrformat, bool warn)
{
  unsigned nset = 0;
  char gtkid[128];
  for (size_t ii = dco.getNumMembers(); ii--;) {
    if (dco.getMemberArity(ii) == Single) {
      snprintf(gtkid, sizeof(gtkid), format, dco.getMemberName(ii));
      boost::any b;
      if (_getValue(gtkid, dco, ii, b, warn)) {
        nset++;
        try {
          dco[ii].write(b);
        }
        catch (dueca::ConversionNotDefined &e) {
          /* DUECA graphics.

               Cannot convert value retrieved from interface into an enumerated
               item value. Check how you set up your interface stores. */
          W_MOD("GtkGladeWindow::getValues: Cannot convert value \""
                << boost::any_cast<std::string>(b) << "\" from widget \""
                << gtkid << "\"");
        }
      }
    }

    else if (dco.getMemberArity(ii) == FixedIterable) {
      if (arrformat != NULL) {
        auto ewriter = dco[ii];
        unsigned idx = 0;
        while (!ewriter.isEnd()) {
          snprintf(gtkid, sizeof(gtkid), format, dco.getMemberName(ii), idx++);
          boost::any b;
          if (_getValue(gtkid, dco, ii, b, warn)) {
            nset++;
            ewriter.write(b);
          }
          else {
            ewriter.skip();
          }
        }
      }
      else {
          /* DUECA graphics.

       You have an array member in the DCO object you try to
       connect to a gtk window, but have not supplied an array
       format string.
    */
        W_XTR("GtkGladeWindow::getValues: No format specified for array member "
              << dco.getMemberName(ii));
      }
    }
    else if (dco.getMemberArity(ii) == Iterable) {
      if (arrformat != NULL) {
        auto ewriter = dco[ii];
        unsigned idx = 0;
        while (true) {
          snprintf(gtkid, sizeof(gtkid), format, dco.getMemberName(ii), idx++);
          boost::any b;
          if (_getValue(gtkid, dco, ii, b, warn)) {
            ewriter.write(b);
          }
          else {
            break;
          }
        }
      }
      else {
          /* DUECA graphics.

       You have an array member in the DCO object you try to
       connect to a gtk window, but have not supplied an array
       format string.
    */
        W_XTR("GtkGladeWindow::getValues: No format specified for array member "
              << dco.getMemberName(ii));
      }
    }
    else {
        /* DUECA graphics.

     This member class (mapping, variable size array or nested) cannot
     be used in connecting to a gtk interface.
  */
      W_XTR("GtkGladeWindow::getValues: Could not interpret organisation of "
            "member "
            << dco.getMemberName(ii));
    }
  }
  return nset;
}

static const GtkGladeWindow::OptionMapping *
_searchMapping(const GtkGladeWindow::OptionMappings *mappings, const char *key,
               bool warn)
{
  if (!mappings) {
    return NULL;
  }
  for (auto m = mappings; m->dcomember != NULL; m++) {
    if (!strcmp(m->dcomember, key)) {
      return m->mapping;
    }
  }
  if (warn) {
      /* DUECA graphics.

     In the given key is missing from the option string mapping for
     selecting an Enum with a ComboBox. Check the mapping against
     the DCO definition for the enum.
  */
    W_XTR("GtkGladeWindow::fillOptions: Mapping for member \""
          << key << "\" not given in options mapping");
  }
  return NULL;
}

bool GtkGladeWindow::fillOptions(const char *dcoclass, const char *format,
                                 const char *arrformat,
                                 const OptionMappings *mappings, bool warn)
{
  auto eclass = DataClassRegistry::single().getEntryShared(dcoclass);
  if (!eclass.get()) {
      /* DUECA graphics.

     When trying to fill selections for combobox entries in a GUI,
     (GtkGladeWindow::fillOptions), the specified dco data class is
     not available. Check spelling, or add the class to the
     executable.
  */
    E_XTR("GtkGladeWindow cannot access data class " << dcoclass);
    return false;
  }

    // work variable
  char gtkid[128];
  auto converter = DataClassRegistry::single().getConverter(dcoclass);
  void *object = converter->clone(NULL);

    /** Run through all members. */
  for (size_t im = 0;
       im < DataClassRegistry::single().getNumMembers(eclass.get()); im++) {
    auto access =
      DataClassRegistry::single().getMemberAccessor(eclass.get(), im);

      // only the enums
    if (access->isEnum()) {
        // reader and writer are used to find enum names
      auto eltreader = access->getReader(object);
      auto eltwriter = access->getWriter(object);

        // iterable, run through the
      if (access->getArity() == FixedIterable) {
        if (arrformat != NULL) {
          for (unsigned idx = access->getSize(); idx--;) {
            snprintf(gtkid, sizeof(gtkid), arrformat, access->getName(), idx);
              // now need to get the enum values?
            _fillOptions(gtkid, eltwriter, eltreader,
                         _searchMapping(mappings, access->getName(), warn),
                         warn);
          }
        }
        else {
            /* DUECA graphics.

           There is an enum array specified, but no array format
           available for finding it in the interface.
        */
          W_XTR("GtkGladeWindow::fillOptions missing array format");
        }
      }
      else if (access->getArity() == Single) {

        snprintf(gtkid, sizeof(gtkid), format, access->getName());
          // again, enum values
        _fillOptions(gtkid, eltwriter, eltreader,
                     _searchMapping(mappings, access->getName(), warn), warn);
      }
    }
  }

    // return the memory
  converter->delData(object);

  return true;
}

DUECA_NS_END
