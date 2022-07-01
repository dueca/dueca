/* ------------------------------------------------------------------   */
/*      item            : GtkDuecaButtons.cxx
        made by         : Rene' van Paassen
        date            : 060424
        category        : body file
        description     :
        changes         : 060424 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define GtkDuecaButtons_cxx
#include "GtkDuecaButtons.hxx"

#include <DuecaPath.hxx>

static void toggle_visibility(GtkWidget* widget, gpointer data)
{
  if ( (*((int*)data))-- == 0) {
    gtk_widget_show(widget);
  }
  else {
    gtk_widget_hide(widget);
  }
}


void gtk_dueca_button_set_image(GtkWidget* widget, gint imgno)
{
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_CONTAINER(widget));
  gtk_container_foreach(GTK_CONTAINER(gtk_bin_get_child(GTK_BIN(widget))),
                        toggle_visibility, (gpointer) &imgno);
}

static GtkWidget* dummyWindow()
{
  static GtkWidget* dummy_window = NULL;
  if (dummy_window == NULL) {
    // a convenience window to obtain default properties for pixmap
    // loading
    dummy_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_realize(dummy_window);
  }
  return dummy_window;
}


GtkWidget* NewGtkDuecaButton_pixmaps()
{
  GdkBitmap *mask = NULL;
  GdkPixmap *pm;
  GtkWidget *pixmap;
  GtkContainer *box = GTK_CONTAINER(gtk_hbox_new(FALSE, 0));

  // inactive pixmap
  pm = gdk_pixmap_create_from_xpm
    (gtk_widget_get_window(dummyWindow()), &mask,
      &(gtk_widget_get_style(dummyWindow())->bg[GTK_STATE_NORMAL]),
      DUECA_NS ::DuecaPath::prepend("pixmaps/inactive.xpm").c_str());
  pixmap = gtk_image_new_from_pixmap(pm, mask);
  gtk_container_add(box, pixmap);
  gtk_widget_show(pixmap);

   // progress pixmap
  pm = gdk_pixmap_create_from_xpm
    (gtk_widget_get_window(dummyWindow()), &mask,
     &(gtk_widget_get_style(dummyWindow())->bg[GTK_STATE_NORMAL]),
     DUECA_NS ::DuecaPath::prepend("pixmaps/inprogress.xpm").c_str());
  gtk_container_add(box, gtk_image_new_from_pixmap(pm, mask));

  // final pixmap
  pm = gdk_pixmap_create_from_xpm
    (gtk_widget_get_window(dummyWindow()), &mask,
     &(gtk_widget_get_style(dummyWindow())->bg[GTK_STATE_NORMAL]),
     DUECA_NS ::DuecaPath::prepend("pixmaps/active.xpm").c_str());
  gtk_container_add(box, gtk_image_new_from_pixmap(pm, mask));

  // broken consistency pixmap
  pm = gdk_pixmap_create_from_xpm
    (gtk_widget_get_window(dummyWindow()), &mask,
     &(gtk_widget_get_style(dummyWindow())->bg[GTK_STATE_NORMAL]),
     DUECA_NS ::DuecaPath::prepend("pixmaps/incomplete.xpm").c_str());
  gtk_container_add(box, gtk_image_new_from_pixmap(pm, mask));

  // fallback pixmap
  pm = gdk_pixmap_create_from_xpm
    (gtk_widget_get_window(dummyWindow()), &mask,
     &(gtk_widget_get_style(dummyWindow())->bg[GTK_STATE_NORMAL]),
     DUECA_NS ::DuecaPath::prepend("pixmaps/fell-back.xpm").c_str());
  gtk_container_add(box, gtk_image_new_from_pixmap(pm, mask));

  // show the box
  gtk_widget_show(GTK_WIDGET(box));

  return GTK_WIDGET(box);
}

GtkWidget* NewGtkDuecaAbortButton_pixmaps()
{
  // make a container with the many faces of this button
  GtkContainer *box = GTK_CONTAINER(gtk_hbox_new(FALSE, 0));

  // Abort pixmap
  GdkPixmap *pm; GdkBitmap *mask; GtkWidget *pixmap;

  pm = gdk_pixmap_create_from_xpm
    (gtk_widget_get_window(dummyWindow()), &mask,
     &(gtk_widget_get_style(dummyWindow())->bg[GTK_STATE_NORMAL]),
     DUECA_NS ::DuecaPath::prepend("pixmaps/abort.xpm").c_str());
  pixmap = gtk_image_new_from_pixmap(pm, mask);
  gtk_container_add(box, pixmap);
  gtk_widget_show(pixmap);

   // Abort confirmation
  pm = gdk_pixmap_create_from_xpm
    (gtk_widget_get_window(dummyWindow()), &mask,
     &(gtk_widget_get_style(dummyWindow())->bg[GTK_STATE_NORMAL]),
     DUECA_NS ::DuecaPath::prepend("pixmaps/confirm-abort.xpm").c_str());
  gtk_container_add(box, gtk_image_new_from_pixmap(pm, mask));
  gtk_widget_show(GTK_WIDGET(box));

  return GTK_WIDGET(box);
}

