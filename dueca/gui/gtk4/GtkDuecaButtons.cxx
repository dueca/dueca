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


void gtk_dueca_button_set_image(GtkWidget* widget, gint imgno)
{
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_BOX(widget));
  GtkWidget *child = gtk_widget_get_first_child(widget);
  gint icount = 0;
  while(child != NULL) {
    gtk_widget_set_visible(child, imgno == icount ? TRUE : FALSE);
    child = gtk_widget_get_next_sibling(child); icount++;
  }
}

GtkWidget* NewGtkDuecaButton_pixmaps()
{
  //GdkBitmap *mask = NULL;
  //cairo_surface_t *pm;
  GtkWidget *pixmap;
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  // inactive pixmap
  //pm = cairo_image_surface_create_from_xpm
  //  (DUECA_NS ::DuecaPath::prepend("pixmaps/inactive.xpm").c_str());

  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/inactive.xpm").c_str());
  gtk_box_append(GTK_BOX(box), pixmap);
  gtk_widget_show(pixmap);

   // progress pixmap
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/inprogress.xpm").c_str());
  gtk_box_append(GTK_BOX(box), pixmap);

  // final pixmap
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/active.xpm").c_str());
  gtk_box_append(GTK_BOX(box), pixmap);

  // broken consistency pixmap
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/incomplete.xpm").c_str());
  gtk_box_append(GTK_BOX(box), pixmap);

  // fallback pixmap
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/fell-back.xpm").c_str());
  gtk_box_append(GTK_BOX(box), pixmap);

  // show the box
  gtk_widget_show(box);

  return box;
}

GtkWidget* NewGtkDuecaAbortButton_pixmaps()
{
  // make a container with the many faces of this button
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  // Abort pixmap
  GtkWidget *pixmap;

  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/abort.xpm").c_str());
  gtk_box_append(GTK_BOX(box), pixmap);
  gtk_widget_show(pixmap);

   // Abort confirmation
  pixmap = gtk_image_new_from_file
    (DUECA_NS ::DuecaPath::prepend("pixmaps/confirm-abort.xpm").c_str());
  gtk_box_append(GTK_BOX(box), pixmap);
  gtk_widget_show(GTK_WIDGET(box));

  return GTK_WIDGET(box);
}

