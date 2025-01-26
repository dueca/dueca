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


#include "dueca_ns.h"
#define GtkDuecaButtons_cxx
#include "GtkDuecaButtons.hxx"

#include <DuecaPath.hxx>
#include <cassert>
#include <debug.h>

DUECA_NS_START;

static GdkPaintable *emergency_icons[2] = { NULL, NULL };
static GdkPaintable *button_icons[5] = { NULL, NULL, NULL };

inline GdkPaintable *loadTextureFromFile(const char *fname)
{
  GError *error = NULL;
  auto tex = gdk_texture_new_from_filename(
    DuecaPath::prepend(fname).c_str(), &error);
  if (error) {
    /* DUECA UI.

       Cannot load icon texture. Check DUECA installation.
    */
    E_XTR("Could not load texture \"" << DuecaPath::prepend(fname) << "\": " << error->message);
    g_error_free(error);
    error = NULL;
    return NULL;
  }
  g_object_ref(G_OBJECT(tex));
  return GDK_PAINTABLE(tex);
}

void load_dueca_buttons()
{
  // load the emergency and button icons
  if (!emergency_icons[0]) {
    emergency_icons[0] = loadTextureFromFile("pixmaps/abort.svg");
    emergency_icons[1] = loadTextureFromFile("pixmaps/confirm-abort.svg");
    button_icons[0] = loadTextureFromFile("pixmaps/inactive.svg");
    button_icons[1] = loadTextureFromFile("pixmaps/inprogress.svg");
    button_icons[2] = loadTextureFromFile("pixmaps/active.svg");
    button_icons[3] = loadTextureFromFile("pixmaps/incomplete.svg");
    button_icons[4] = loadTextureFromFile("pixmaps/fell-back.svg");
  }
};

void gtk_dueca_button_load_image(GtkWidget *btn, unsigned imno)
{
  assert(imno < 5);
  auto img = GTK_PICTURE(gtk_widget_get_first_child(btn));
  //gtk_image_set_icon_size(img, GTK_ICON_SIZE_LARGE);
  gtk_picture_set_paintable(img, button_icons[imno]);
}

void gtk_dueca_emergency_load_image(GtkWidget *btn, unsigned imno)
{
  assert(imno < 2);
  auto img = GTK_PICTURE(gtk_widget_get_first_child(btn));
  //gtk_image_set_icon_size(img, GTK_ICON_SIZE_LARGE);
  gtk_picture_set_paintable(img, emergency_icons[imno]);
}

DUECA_NS_END;

