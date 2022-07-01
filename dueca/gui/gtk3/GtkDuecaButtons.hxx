/* ------------------------------------------------------------------   */
/*      item            : GtkDuecaButtons.hxx
        made by         : Rene van Paassen
        date            : 060424
        category        : header file
        description     : This defines a Gtk toggle button
        (basically), with several alternative bitmap images. The
        bitmap images are used to indicate progress of verifying DUECA
        state.
        changes         : 060424 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GtkDuecaButtons_hxx
#define GtkDuecaButtons_hxx

#include <dueca-conf.h>
#include <dueca_ns.h>

//#include <gdk/gdk.h>
#include <gtk/gtk.h>

/** Change the background image of the GtkDuecaButton or GtkDuecaAbortButton.
    \param widget  Pointer to the button widget.
    \param imgno   Selected image. */
extern "C" {
  void gtk_dueca_button_set_image(GtkWidget* widget, gint imgno);
}

/** Load a set of widgets for the abort button. */
GtkWidget* NewGtkDuecaAbortButton_pixmaps();

/** Load a set of widgets for the dueca buttons. */
GtkWidget* NewGtkDuecaButton_pixmaps();

#endif
