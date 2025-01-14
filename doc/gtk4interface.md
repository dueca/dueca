# Interface windows with GTK4 {#gtk4}

## Introduction

DUECA currently provides facilities to create experiment interfaces (for operating your simulation or experiment), with gtk2, gtk3, and gtk4. The gtk2 interfacing is really obsolete, and it is currently difficult to find programs (old versions of the "glade" program specifically), to design these interfaces. So DO NOT USE THIS ANY MORE!

The gtk3 interfacing library is still well supported, and you can use the "glade" program to create your interfaces in a graphical manner, and when completed, these files can be used directly by DUECA to create interface windows. However, gtk3 is quickly being phased out, and to keep compatibility with the future, it is better to use gtk4.

The fourth version of the gtk toolkit, gtk4, looks a lot like later versions gtk3, but there are a number of important changes:

- In gtk4, *almost everything* is a normal widget, except for the menu system. Gtk3 still had different types of widgets for the toolbar (ToolButton, RadioToolButton, etc.). Gtk4 removed all of this stuff, just use a normal box, with normal widgets, and give the box a "toolbar" style. 

- The same thing happened for list views and tree views. In gtk3, you had to indicate which type of column you wanted in one of those views, and give gtk a table with data to fill the fields in these columns. In gtk4 you need to create a "factory", that in turn creates and fills widgets for in the table. It looks a bit complicated (and examples are still scarce), so I added a section to this documentation that lays this process out step-by-step. 

- Another big change is the menu system. In gtk4 you can easily define a menu by its layout. However it won't do anything until you connected your menu items to "activities". The way in which you connect these items determines whether your menu items are normal selections, have checkboxes to be toggled, or function like radio buttens in selecting one option from a number of possible choices. Another section lays out step-by-step instructions for that as well. 

- The old and trusty "glade" program is not updated to gtk4, and its alternatives are either not complete yet ("Cambalache"), or work in a slightly different way ("Workbench"). I will outline the use of Workbench for building user interfaces. 

## Workbench

You can install workbench by Sonny Piers using flatpack:

    flatpack flatpak install flathub re.sonny.Workbench

Run it using 

    flatpack run re.sonny.Workbench

Workbench lets you create an interface specification with a simple script editor. While creating the specification, Workbench can show you the resulting interface. 

Ctrl-N will give you a new project, Ctrl-O gives a window to open an existing project. Check the library for examples. While you edit the "Blueprint" specification of your interface, the file with the .ui extension will also be written. That file can then directly be read by DUECA's GtkGladeWindow.

To figure out how to use Workbench, look at the examples in its library. 

## Simple button linking



## Sucking the data in to fill a DCO object

## Making a menu

### Normal menu item

### Toggle/checkbox type

### Radiobutton type menu items

## List view

### Creating your data type

### Filling or binding?

## Tree view

### How are trees expanded

