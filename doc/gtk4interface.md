# Interface windows with GTK4 {#gtk4}

## Introduction

DUECA currently provides facilities to create experiment interfaces (for operating your simulation or experiment), with gtk2, gtk3, and gtk4. The gtk2 interfacing is really obsolete, and it is currently difficult to find programs (old versions of the "glade" program specifically), to design these interfaces. So DO NOT USE THIS ANY MORE!

The gtk3 interfacing library is still well supported, and you can use the "glade" program to create your interfaces in a graphical manner, and when completed, these files can be used directly by DUECA to create interface windows. However, gtk3 is quickly being phased out, and to keep compatibility with the future, it is better to use gtk4.

The fourth version of the gtk toolkit, gtk4, looks a lot like later versions gtk3, but there are a number of important changes:

- In gtk4, *almost everything* is a normal widget, except for the menu system. Gtk3 still had different types of widgets for the toolbar (ToolButton, RadioToolButton, etc.). Gtk4 removed all of this stuff, and if you want a toolbar, just use a normal box, with normal widgets, and give the box a "toolbar" style. 

- The same thing happened for "list" views and "tree" views. In gtk3, you can use a `GtkTreeView` to show data in a list with tree expansion. To provide data for this view, the data has to be copied to a "tree model", which could hold typical types of data (strings, numbers), in a table format. In gtk4 all that is gone. You no longer need to copy your data into a gtk-compatible table, instead you need to create a "factory", that in its turn creates widgets (normal widgets) to be shown in the table, and can link the data in those widgets to the data in your program. It looks a bit complicated (and examples are still scarce), so I added a section to this documentation that lays this process out step-by-step. 

- Another big change is the menu system. In gtk4 you can easily define a menu by its layout. However it won't do anything until you connected your menu items to "activities". The way in which you connect these items determines whether your menu items are shown as normal selections, have checkboxes to be toggled, indicating the state of the menu, or function like radio buttons in that you can select and activate one option from a number of possible choices. Another section lays out step-by-step instructions for that as well. 

- The old and trusty "glade" program is not updated to gtk4, and its alternatives are either not complete yet ("Cambalache"), or work in a slightly different way ("Workbench"). I will outline the use of Workbench for building user interfaces. 

## Workbench

You can install workbench by Sonny Piers using flatpack:

    flatpak install flathub re.sonny.Workbench

Run it using 

    flatpak run re.sonny.Workbench

Workbench lets you create an interface specification with a simple script editor. While creating the specification, Workbench can show you the resulting interface.

Ctrl-N will give you a new project, Ctrl-O gives a window to open an existing project. Each project is in its own folder, so you need a folder for each graphical interface you want to create. There are a number of files created in such a folder:

- `main.blp` This is the "BluePrint" format file for defining your interface. The BluePrint format is really compact, and this is the main way in which you type up your interface. Create widgets and give them properties (and data, if applicable); Workbench will check whether the syntax and writing are correct. 

- `main.ui` A direct translation of everything in `main.blp` (given that your syntax is correct), to an xml format that can be interpreted by the gtk builder. This file will be used by your program.

- `settings` and `jsconfig.json`, two additional files used by Workbench.

When using Workbench, check its library for examples. These examples give you an idea of what is possible, and how to implement many common interface elements. You can ask for a live preview of your interface, and see how it works.  

## Showing an interface

To show one of these interfaces, created with Workbench or some other tool (Cambalache), you can use the GtkGladeWindow class provided by DUECA. In your module, add a member of this class:

~~~~{.cxx}
class ExperimentInterface: public Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ExperimentInterface _ThisModule_;

  // ... snip
  GtkGladeWindow eciwindow;
  // ... snip
};
~~~~

When it is time to open the interface, usually in the `complete` method, give it the location of the `.ui` file:

~~~~{.cxx}
bool ExperimentInterface::complete()
{
  // this creates the gtk window (or whatever widget is identified by "mainwindow")
  eciwindow.readGladeFile("../../../../MyProject/experiment-interface/interface/main.ui",
    "mainwindow");

  // and actually show it
  eciwindow.show();

  // happy
  return true;
}
~~~~

Of course, now you have the window, but what can you do with it? For that, read the next section. 

## Simple button linking

The widgets that you use in your interface all have their own possibilities to react to user input. Look at the [GTK4 Documentation](https://docs.gtk.org/gtk4) for the details. Most widgets have "signals" that can be emitted by the widget, and these can be used to trigger actions in your code (DUECA module). For example a normal [button](https://docs.gtk.org/gtk4/class.Button.html), can produce a "clicked" signal. Suppose that the `mainwindow` in the previous example has a button, and its `BluePrint` code looks like the following:

~~~~{.blp}
using Gtk 4.0;

Window mainwindow {
  
  title: "An experiment interface";

  Box {
    orientation: vertical;

    Button my_button {
      label: "Oh, please press me";
    }

    Label my_label {
      label: "Count: 0";
    }

    Entry eci_participantid {
      placeholder-text: "ID";
    }
  }
}
~~~~

![The example interface](interfacegtk4.png)

The window will have an id `mainwindow`, which will be important in telling GtkGladeWindow which widget to create as main widget, and the button has been labeled `my_button`. With the label, it will be accessible from the code.

To make sure that the "clicked" signal from the `my_button` arrives at our code, we need to link it to a function of the `ExperimentInterface` module. In your header, add this function; to see what it should look like, check the documentation for the ["clicked" signal](https://docs.gtk.org/gtk4/signal.Button.clicked.html):

~~~~{.cxx}
private:

  // a counter?
  unsigned count;

  // react to button presses
  void cbMyButton(GtkButton* button, gpointer user_data);
~~~~

Instead of opening the window without further ado, we will use a callback table to tell GtkGladeWindow how the signal from the button is to be linked to the code, so re-write the `complete` function as:

~~~~{.cxx}
bool ExperimentInterface::complete()
{
  // table linking widget signals to code. 
  static GladeCallbackTable cb_links[] = {
    { "my_button", "clicked", gtk_callback(&_ThisModule_::cbMyButton) },
    // IT HAS TO BE CLOSED OFF WITH NULL!
    { NULL }
  };

  // this creates the gtk window (or whatever widget is identified by "mainwindow")
  eciwindow.readGladeFile("../../../../MyProject/experiment-interface/interface/main.ui",
    "mainwindow", this, cb_links);

  // and actually show it
  eciwindow.show();

  // happy now
  return true;
}
~~~~

Note that in the call to `readGladeFile`, now the `this` pointer for the module, and the callback table are passed. With this data, the `readGladeFile` function will connect all signals specified in the table to the newly created widgets in the interface. Whenever the button is now clicked, the `cbMyButton` function will be called.

Initialize the counter to zero in the constructor of your `ExperimentInterface` module. As an example for the button callback function, we could make this:

~~~~{.cxx}
void ExperimentInterface::cbMyButton(GtkButton* button, gpointer user_data)
{
  // increase the counter
  count += 1;

  // write a string:
  auto newlabel = boost::str(boost::format("Count: %d") % count);

  // set this label text in the label widget
  gtk_label_set_label(GTK_LABEL(eciwindow["my_label"]), newlabel.c_str());
}
~~~~

We can see a number of things here. First, I used the code in the handy `boost/format.hpp` to write a new label. The `eciwindow` object has a handy indexing function to look up widgets in the interface, we use that to get the label widget. This returns a `GtkWidget` object, the `GTK_LABEL` macro will convert/cast that one to a `GtkLabel`, which can then be used to change the label text. The c-style interface for the gtk widget toolkit might be a bit cumbersome, but it is precise and does a lot of checking, and personally I prefer this to using the c++ interface for it.

## Sucking the data in to fill a DCO object

Of course, reacting to single button presses is not the only way to get data from the interface into your program. In most cases when making an interface like this, you want to collect the input on the interface (participant number, experiment condition to be used, options, etc.) into a DCO object that you can send around as an experiment or simulation configuration event. There are some handy functions built into the `GtkGladeWindow` object to help you there. Let's assume we have the following minimalistic DCO object:

~~~~{.scm}
(Type std::string "#include <string>")

(Object ExpCondition
    (std::string participantid)
)
~~~~

Also assume you have defined an event write token `w_expcond` (left as exercise for the reader). We can now use the last element in the display, the text entry box, to show how data can be obtained from the interface and collected in a DCO object. Modify the button callback to:

~~~~{.cxx}
void ExperimentInterface::cbMyButton(GtkButton* button, gpointer user_data)
{
  // increase the counter
  count += 1;

  // write a string:
  auto newlabel = boost::str(boost::format("Count: %d") % count);

  // set this label text in the label widget
  gtk_label_set_label(GTK_LABEL(eciwindow["my_label"]), newlabel.c_str());

  // now collect and send the "experiment condition" data
  // no timespec given (don't know that), so it will be sent with current time
  DCOWriter we(w_expcond);
  eciwindow.getValues(we, "eci_%s", NULL, true);
}
~~~~

Some explanation on how this works might be in order. The `DCOWriter` object works as the `DataWriter` you should be familiar with, however it can write the `ExpCondition` object without actually knowing what an `ExpCondition` object is, through introspection. This writer is given to the `eciwindow`'s `getValues` call. This call inspects both the DCO object and the interface, and then tries to fill all "matching" members of the DCO object, in this case an `ExpCondition`. 

The `"eci_%s"` string specifies the format for looking for widgets. It will be combined with the name of the members in the DCO object (in this case the single member of the `ExpCondition`), here resulting in `"eci_participantid"`. That is the name of the `GtkEntry` widget, so that will be matched, and the text found in the `GtkEntry` widget will be written in the `ExpCondition` object that will be sent off over the channel linked to `w_expcond`. By using a prefix (in this case "eci_"), we can avoid confusion with names of other widgets, and it will also be possible to extract multiple identical DCO object from the interface, as long as different prefixes are used in naming the widgets. 

It is also possible to work the other way, with a `setValues` call, and set values from a DCO object in the interface. For a short overview of what types of data can be matched to what types of widgets, see the table below:

| data type   | widgets                                                          |
| ----------- | -----------------------------------------------------------------|
| std::string | GtkEntry, GtkDropDown                                            |
| double      | GtkEntry, GtkSpinButton, GtkDropDown (with numbers), GtkRange    |
| float       | GtkEntry, GtkSpinButton, GtkDropDown (with numbers), GtkRange    |
| int8_t..int64_t | GtkEntry, GtkSpinButton, GtkDropDown (with numbers), GtkRange    |
| uint8_t..uint64_t  | GtkEntry, GtkSpinButton, GtkDropDown (with numbers), GtkRange    |
| enum        | GtkDropDown (with enum values), GtkCheckButton (as radiobutton)  |
| bool        | GtkCheckButton, GtkToggleButton                                  |

The GtkDropDown widgets can be loaded with values with the `loadDropDownText` call, for example to load a dropdown box with names of experimental conditions. The `fillOptions` text call can be used to fill dropdowns with all possible enum values.

Linking an enum to a group of radio buttons needs some special preparation. In gtk4, radio buttons are created as GtkCheckButton widgets, which are then connected by setting the "group" property of all but one of these checkboxes to the id of the main checkbutton. To link these with the enum values, you need to specify these in the widget id; like so:

~~~~{.blp}
  CheckButton eci_option-Default {
    label: "Default option";
    active: true;
  }

  CheckButton eci_option-HiGain {
    label: "High-gain condition";
    group: eci_option-Default;
  }

  CheckButton eci_option-EasyDoesIt {
    label: "Low gain and careful";
    group: eci_option-Default;
  }
~~~~

This would assume an prefix of `eci_`, then an enum in the `option` member of your DCO which can have values `Default`, `HiGain` and `EasyDoesIt`. Depending on which of these radio buttons is the currently active one, the enum value will be set.


## Making a menu

In gtk4, menus are shown by specific widgets, such as the MenuButton, or attached to the whole application. The items in 
menus must be linked to actions to be enabled. 

Further details for this section to be written.

### Normal menu item

### Toggle/checkbox type

### Radiobutton type menu items

## List view

Tables with data are often useful for showing experiment results, or grouping a large number of similar options/data together. When the table size is not known beforehand (i.e., a variable number of rows will be produced), you can use the GtkColumnView to present this data. This is one of the big changes compared to gtk3, and although the resulting solution at first look seems more complicated, after having converted a number of these I can say that the gtk4 version is more robust, flexible, and it can be approached in a simple, step-wise manner. 

### Creating your data type

If you want to use a table with multiple columns in gtk4, you should define a data type to be used in that table. That data type needs to interact with the gtk4 system. In all likelihood you have already collected/joined your data in C++ classes (if you use OO design). It is best to make the datatype you create a thin shell to connect to your existing data. For argument's sake, lets suppose you created a DCO object with some stuff:

~~~~{.scm}
(Type float)
(Type uint32_t)
(Type std::string "#include <string>")
(Enum Condition uint8_t Easy Moderate Hard)
(Object Results
    (std::string participantid)
    (uint32_t runnumber (Default 0))
    (float score (Default 0.0f))
    (Condition condition (Default Easy))
)
~~~~

And as data comes in, you store these in a list, using a `std::shared_ptr`:

~~~~{.hxx}
  // somewhere in your class/module header
  typedef std::list<std::shared_ptr<Results>> resultlist_t;

  resultlist_t results;
~~~~

You need a datatype that is as "thin" as possible, compatible with the gtk (actually g) object system. Let's make the following, typically in your .cxx file, since you will use it only there:

~~~~{.cxx}
// define the struct for my data type
struct _MyResults 
{
  // extremely important to have a GObject (or possibly GInitiallyUnowned) as parent
  GObject parent;
  
  // and simply link to my data somewhere else
  std::shared_ptr<Results> result;
};
~~~~

For this example, let's assume that the score will be updated during the run (so its value in the table will change). To do that, we need to define the score as a property on the new datatype we are going to define. Create an enum for remembering that.

~~~~{.cxx}
// Definition of my properties
// Important: the first property should start with 1
enum MyResultsProperty { MY_RESULTS_SCORE = 1, MY_RESULTS_NPROPERTIES };

// at this point use some macros to define the new type (need the result below)
G_DECLARE_FINAL_TYPE(MyResults, my_results, MY, RESULTS, GObject);
G_DEFINE_TYPE(MyResults, my_results, G_TYPE_OBJECT);

// for a new object type, you need a function that sets the properties, and one that gets them
// the function that sets the property will not be used!
static void my_results_set_property(Object *object, guint property_id,
                                    const GValue *value, GParamSpec *pspec)
{ }

// this one will be used
static void my_results_get_property(GObject *object, guint property_id,
                                     GValue *value, GParamSpec *pspec)
{
  MyResults *self = MY_RESULTS(object);
  switch (static_cast<MyResultsProperty>(property_id)) {
  case MY_RESULTS_SCORE:
    g_value_set_float(value, self->results->score);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
  }
}

// need to define the properties that our new type has
static GParamSpec *my_results_properties[MY_RESULTS_NPROPERTIES] = {
  NULL,  // again, because my first prop starts with 1!
  g_param_spec_float("score", "Score", "Participant score", 0.0, 1E10, 0.0,
                     static_cast<GParamFlags>(G_PARAM_READWRITE |
                                              G_PARAM_EXPLICIT_NOTIFY |
                                              G_PARAM_CONSTRUCT))
};

// if an object of my data type is removed, we NEED TO REMOVE THE LINK
static void my_results_dispose(GObject *object)
{
  auto self = MY_RESULTS(object);
  // reset the shared pointer!
  self->result.reset();
}

// we need a class init and an object init
static void my_results_class_init(MyLocationClass *_klass)
{
  auto klass = G_OBJECT_CLASS(_klass);
  klass->set_property = my_results_set_property;
  klass->get_property = my_results_get_property;
  klass->dispose = my_results_dispose;
  g_object_class_install_properties(klass, G_N_ELEMENTS(my_results_properties),
                                    my_results_properties);
}

// no code needed here
static void my_results_init(MyResults *_self)
{ }

// this is all, however it is a good practice to create an initialization function
static MyResults *my_results_new(const std::shared_ptr<Results>& p)
{
  auto res = MY_RESULTS(g_object_new(my_results_get_type()), NULL);
  // rest of the memory was zeroed by gtk / gio. That is apparently OK for
  // a shared_ptr, but to be absolutely sure, use "placement new"
  new(&(res->result)) std::shared_ptr<Results>(p);
}
~~~~

I hope most of this stuff is clear enough, and can be adjusted for your purpose. The "placement new" example is a safe way to initialize the data; for simple stuff (integers, floats, doubles etc.) it will not be needed.

### Filling or binding?

The above give a new type that is a shallow link between your data and the gtk4 world. With this, we can define a piece of interface code with Workbench that should be able to show the data.

~~~~{.blp}
using Gtk 4.0;

Window mainwindow {

  ScrolledWindow {
    
    ColumnView results_table {
        
      ColumnViewColumn {
        title: "Participant";
        factory: SignalListItemFactory fact_participant {};
      }
      
      ColumnViewColumn {
        title: "Condition";
        factory: SignalListItemFactory fact_condition {};
      }
      
      ColumnViewColumn {
        title: "Score";
        factory: SignalListItemFactory fact_score {};
      }
    
    }
  }

}
~~~~

It has a number of typical components:

- The ColumnView, and we gave it a name, because we need to find it and link it to our data
- Three factories, which will create the widgets that are shown in the column view, and will link our data to these widgets.

The factories have two typical signals that need to be connected, "setup" and "bind". The ColumnView code is smart, in that it will create enough cells through the factories to fill the rows that are visible on the screen, and will fill these cells with the data from your application. To make that happen, you need to create a number of new functions for your class/module.

~~~~{.hxx}
  // somewhere in your class (look up format in documentation!)
  void cbSetupLabel(GtkSignalListItemFactory *fact,
                    GtkListItem *item, gpointer user_data);

  void cbBindParticipantId(GtkSignalListItemFactory *fact,
                           GtkListItem *item, gpointer user_data);
  void cbBindCondition(GtkSignalListItemFactory *fact,
                           GtkListItem *item, gpointer user_data);
  void cbBindScore(GtkSignalListItemFactory *fact,
                           GtkListItem *item, gpointer user_data);
~~~~

All three columns will be filled with label fields, and we can use the same setup function for those. However, the data connection differs, and we use three different bind functions. 

In the `complete()` function, we need a new callback table. Look above for how to use it:

~~~~{.cxx}
  // table linking widget signals to code. 
  static GladeCallbackTable cb_links[] = {
    { "fact_participant", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "fact_condition", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "fact_score", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },

    { "fact_participant", "bind", gtk_callback(&_ThisModule_::cbBindLabel) },
    { "fact_condition", "bind", gtk_callback(&_ThisModule_::cbBindCondition) },
    { "fact_score", "bind", gtk_callback(&_ThisModule_::cbBindScore) },

    // IT HAS STILL TO BE CLOSED OFF WITH NULL!
    { NULL }
  };

  window.readGladeFile("../../../show-score/score-interface/main.ui",
                       "welcome", this, cb_links);
 
  // in addition, we need to give the table a list of our freshly-minted objects
  auto table = GTK_COLUMN_VIEW(window["results_table"]);

  // this is a GListStore object, defined in your class
  result_store = g_list_store_new(my_results_get_type());

  // different selection types are possible. No selection here
  auto selection = gtk_no_selection_new(G_LIST_MODEL(result_store));
  gtk_column_view_set_model(table, GTK_SELECTION_MODEL(selection));

  window.show();
~~~~

The label set-up function is simple, it creates a new label object, and gives it to the list item.

~~~~{.cxx}
void ResultsWindow::cbSetupLabel(GtkSignalListItemFactory *fact,
                                 GtkListItem *item, gpointer user_data)
{
  auto label = gtk_label_new("");
  gtk_list_item_set_child(item, label);
}
~~~~

The bind functions should link the data in your application to the interface widgets. If the data does not change (like the participant id), you can simply copy it over by setting the label string:

~~~~{.cxx}
void ResultsWindow::cbBindParticipantId(GtkSignalListItemFactory *fact, GtkListItem *item,
                                        gpointer user_data)
{
  // name does not change, simply setting the string in the label is OK
  auto label = gtk_list_item_get_child(item);
  auto obj = MY_RESULTS(gtk_list_item_get_item(item));
  gtk_label_set_label(GTK_LABEL(label), obj->result->participantid.c_str());
}
~~~~

However, as an example, let's say we start an experiment, and the running score (for the same participant/run number) is regularly updated. To do that, you can create a dynamic binding, and that is why we created a "score" property on the new data type. 

~~~~{.cxx}
void ResultsWindow::cbBindScore(GtkSignalListItemFactory *fact, GtkListItem *item,
                                gpointer user_data)
{
  // score changes, make a bind
  auto label = gtk_list_item_get_child(item);
  auto obj = MY_RESULTS(gtk_list_item_get_item(item));
  g_object_bind_property(obj, "score", label, "label", G_BINDING_DEFAULT);
}
~~~~

Now, in your `doCalculation` activity, you can check whether new data comes in for the score (assuming you created a channel for that), and if the run number changed, create a new entry in the list of results, or, if it did not change, update the score that you have. Here is some example code:

~~~~{.cxx}
  // somewhere in doCalculation
  while (r_results.haveVisibleSets(ts)) {

    DataReader<Results> r(r_results, ts);

    if (!results.size() || (results.back()->runnumber != r.data().runnumber)) {
      
      // new run, push back to my list of results, and update the store
      auto newresult = std::shared_ptr<Results>(new Results(r.data()));

      // data in this module
      results.push_back(newresult);

      // extend the list, the factories will now start working
      auto gresult = my_results_new(newresult);
      g_list_store_append(result_store, gresult);
    }
    else {
    
      // just update the data from the last result
      results.back()->score = r.data().score;

      // this corresponds to the last element in the list store
      auto nelts = g_list_store_get_n_items(result_store);
      auto lastelt = g_list_store_get_item(result_store, nelts-1);

      // notify it has changed
      g_object_notify_by_pspec(G_OBJECT(lastelt), 
                               my_results_properties[MY_RESULTS_SCORE]);
    }

  }
~~~~

## Tree view

TO BE COMPLETED.

### How are trees expanded

