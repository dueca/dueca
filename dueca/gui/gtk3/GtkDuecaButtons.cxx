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

GtkWidget* NewGtkDuecaButton_pixmaps()
{
  //GdkBitmap *mask = NULL;
  //cairo_surface_t *pm;
  GtkWidget *pixmap;
  GtkContainer *box = GTK_CONTAINER(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

  // inactive pixmap
  //pm = cairo_image_surface_create_from_xpm
  //  (DUECA_NS ::DuecaPath::prepend("pixmaps/inactive.xpm").c_str());

  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/inactive.xpm").c_str());
  gtk_container_add(box, pixmap);
  gtk_widget_show(pixmap);

   // progress pixmap
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/inprogress.xpm").c_str());
  gtk_container_add(box, pixmap);

  // final pixmap
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/active.xpm").c_str());
  gtk_container_add(box, pixmap);

  // broken consistency pixmap
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/incomplete.xpm").c_str());
  gtk_container_add(box, pixmap);

  // fallback pixmap
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/fell-back.xpm").c_str());
  gtk_container_add(box, pixmap);

  // show the box
  gtk_widget_show(GTK_WIDGET(box));

  return GTK_WIDGET(box);
}

GtkWidget* NewGtkDuecaAbortButton_pixmaps()
{
  // make a container with the many faces of this button
  GtkContainer *box = GTK_CONTAINER(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

  // Abort pixmap
  GtkWidget *pixmap;

  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/abort.xpm").c_str());
  gtk_container_add(box, pixmap);
  gtk_widget_show(pixmap);

   // Abort confirmation
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/confirm-abort.xpm").c_str());
  gtk_container_add(box, pixmap);
  gtk_widget_show(GTK_WIDGET(box));

  return GTK_WIDGET(box);
}

