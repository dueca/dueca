# Graphical User Interfaces

## Introduction

In many case, you want a graphical user interface to control your
simulation or experiment. The graphical user interfaces used by DUECA
are in Gtk2 or currently Gtk3. For each program, only one of Gtk2 or
Gtk3 can be used, and thus if you have modules in Gtk2, you are
limited to the Gtk2 version of DUECA. 

Presently (2021), Gtk2 is considered obsolete, and Gtk4 is around the
corner (first release in 2020). Given the pace of adoption, and the
fact that tools and distributions do not yet widely support Gtk4, we
can safely work with Gtk3 for a while. A number of projects will have
to be ported from Gtk2 to Gtk3. At least if these projects used glade
files (and not hand-coded or code-generated gtk code), this should be
pretty easy. This section discusses how user interface can easily be
integrated in DUECA code, and also gives some hints on porting glade
files for Gtk2 to Gtk3.

## GtkGladeWindow

DUECA provides a GtkGladeWindow class, with variants both for Gtk2 and
Gtk3. This is the easiest way of showing a user interface window, and
for connecting that window to your code. Take a peek at the
documentation for dueca.GtkGladeWindow . Commonly you will want to
react to some or all of the widgets (buttons and the like) on the GUI
window. You can define links between the widgets and your code in a
simple table that you pass to the dueca.GtkGladeWindow.readGladeFile
call. Simply write functions in the class that handles your window
with the right signature, and add them to the table.

Here are a number of common call signatures:

### Button clicks (example)

In the table, specify the button name, the signal "clicked", the callback and optionally a user data pointer. Example:

    { "mybutton", "clicked", gtk_callback(&MyClass::handleMyButton), gpointer(1) }

A click callback has the button and the user_data (as you supplied in the table), as arguments:

    void handleMyButton(GtkButton* button, gpointer user_data);

### Toggle button changes

Toggle buttons can be read on the "changed" signal



### Enabling or disabling widgets

If you want a smart interface, you might want to block the elements that cannot be used at some time, e.g., prevent changes of your simulation while it is running, and only allow changes in HoldCurrent mode. You can set widget sensitivity with:

    gtk_widget_set_sensitive(mywidget, TRUE);  // or FALSE for insensitive


### Showing or hiding widgets

The following can be used to show or hide a widget (e.g., a window) and its children:

    gtk_widget_show_all(mywidget);

    gtk_widget_hide(mywidget);


### Other signals, signatures





