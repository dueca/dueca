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

#include <dueca_ns.h>
#include <gtk/gtk.h>

DUECA_NS_START;

/** Set a specific image on the given button 

    @param btn  Button widget
    @param imno Chosen image, 0=normal, 1
  */
void gtk_dueca_button_load_image(GtkWidget *btn, unsigned imno);

/** Set an image on the emergency button

    @param btn  Button widget
    @param imno Chosen image, 0=inactive, 1=in progress, 2=active, 
                3=incomplete, 4=fell back
  */
void gtk_dueca_emergency_load_image(GtkWidget *btn, unsigned imno);

void load_dueca_buttons();

DUECA_NS_END;

#endif
