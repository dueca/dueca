#include <gtk/gtk.h>
#include <iostream>
using namespace std;


#include "../dueca/GtkGladeWindow.hxx"

class SomeClass
{
public:

void
on_custom1_add_accelerator             (GtkWidget       *widget,
                                        guint            accel_signal_id,
                                        GtkAccelGroup   *accel_group,
                                        guint            accel_key,
                                        GdkModifierType  accel_mods,
                                        GtkAccelFlags    accel_flags
                                        )
{ cout << "" << endl; }

gboolean
on_custom1_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event
                                        )
  { cout << "" << endl; return 0; }

gboolean
on_custom1_client_event                (GtkWidget       *widget,
                                        GdkEventClient  *event
                                        )
  { cout << "" << endl; return 0;}

gboolean
on_custom1_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event
                                        )
{ cout << "" << endl;  return 0;}

void
on_custom1_debug_msg                   (GtkWidget       *widget,
                                        gchar           *message
                                        )
{ cout << "" << endl; }

gboolean
on_custom1_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event
                                        )
{ cout << "" << endl; return 0; }

gboolean
on_custom1_destroy_event               (GtkWidget       *widget,
                                        GdkEvent        *event
                                        )
{ cout << "" << endl;  return 0;}

void
on_custom1_drag_begin                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context
                                        )
{ cout << "" << endl; }

void
on_custom1_drag_data_delete            (GtkWidget       *widget,
                                        GdkDragContext  *drag_context
                                        )
{ cout << "" << endl; }

void
on_custom1_drag_data_get               (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time
                                        )
{ cout << "" << endl; }

void
on_custom1_drag_data_received          (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time
                                        )
{ cout << "" << endl; }

gboolean
on_custom1_drag_drop                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time
                                        )
{ cout << "" << endl;  return 0;}

void
on_custom1_drag_end                    (GtkWidget       *widget,
                                        GdkDragContext  *drag_context
                                        )
{ cout << "" << endl; }

void
on_custom1_drag_leave                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time
                                        )
{ cout << "" << endl; }

gboolean
on_custom1_drag_motion                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time
                                        )
{ cout << "" << endl;  return 0;}

void
on_custom1_draw                        (GtkWidget       *widget,
                                        GdkRectangle    *area
                                        )
{ cout << "" << endl; }

void
on_custom1_draw_default                (GtkWidget       *widget
                                        )
{ cout << "" << endl; }

void
on_custom1_draw_focus                  (GtkWidget       *widget
                                        )
{ cout << "" << endl; }

gboolean
on_custom1_enter_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event
                                        )
{ cout << "" << endl;  return 0;}

gboolean
on_custom1_event                       (GtkWidget       *widget,
                                        GdkEvent        *event
                                        )
{ cout << "" << endl;  return 0;}

gboolean
on_custom1_expose_event                (GtkWidget       *widget,
                                        GdkEventExpose  *event
                                        )
{ cout << "" << endl;  return 0;}

gboolean
on_custom1_focus_in_event              (GtkWidget       *widget,
                                        GdkEventFocus   *event
                                        )
{ cout << "" << endl;  return 0;}

void
on_custom1_grab_focus                  (GtkWidget       *widget
                                        )
{ cout << "" << endl; }

gboolean
on_custom1_selection_request_event     (GtkWidget       *widget,
                                        GdkEventSelection *event
                                        )
{ cout << "" << endl;  return 0;}

void
on_custom1_show                        (GtkWidget       *widget
                                        )
{ cout << "" << endl; }

void
on_custom1_size_allocate               (GtkWidget       *widget,
                                        GtkAllocation   *allocation
                                        )
{ cout << "" << endl; }

void
on_custom1_size_request                (GtkWidget       *widget,
                                        GtkRequisition  *requisition
                                        )
{ cout << "" << endl; }

void
on_custom1_state_changed               (GtkWidget       *widget,
                                        GtkStateType     state
                                        )
{ cout << "state_changed" << endl; }

void
on_custom1_style_set                   (GtkWidget       *widget,
                                        GtkStyle        *previous_style
                                        )
{ cout << "style_set" << endl; }

void
on_custom1_unmap                       (GtkWidget       *widget
                                        )
{ cout << "unmap" << endl; }

gboolean
on_custom1_unmap_event                 (GtkWidget       *widget,
                                        GdkEvent        *event
                                        )
{ cout << "unmap_event" << endl; return 0; }

void
on_custom1_unrealize                   (GtkWidget       *widget
                                        )
{ cout << "unrealize" << endl; }

gboolean
on_custom1_visibility_notify_event     (GtkWidget       *widget,
                                        GdkEvent        *event
                                        )
{ cout << "visibility_notify_event" << endl;  return 0;}

void
on_custom1_destroy                     (GtkObject       *object
                                        )
{ cout << "destroy" << endl; }

};

USING_DUECA_NS;

void testv()
{
  return void();
}

void testv2()
{
  return testv();
}

int main()
{
  GladeCallbackTable table[] = {
    { "custom1", "add_accelerator", gtk_callback(&SomeClass::on_custom1_add_accelerator) },
    { "custom1", "button_press_event", gtk_callback(&SomeClass::on_custom1_button_press_event) },
    { "custom1", "client_event", gtk_callback(&SomeClass::on_custom1_client_event) },
    { "custom1", "configure_event", gtk_callback(&SomeClass::on_custom1_configure_event) },
    { "custom1", "debug_msg", gtk_callback(&SomeClass::on_custom1_debug_msg) },
    { "custom1", "delete_event", gtk_callback(&SomeClass::on_custom1_delete_event ) },
    { "custom1", "destroy_event", gtk_callback(&SomeClass::on_custom1_destroy_event) },
    { "custom1", "drag_begin", gtk_callback(&SomeClass::on_custom1_drag_begin) },
    { "custom1", "drag_data_delete", gtk_callback(&SomeClass::on_custom1_drag_data_delete) },
    { "custom1", "drag_data_get", gtk_callback(&SomeClass::on_custom1_drag_data_get) },
    { "custom1", "drag_data_received", gtk_callback(&SomeClass::on_custom1_drag_data_received) },
    { "custom1", "drag_drop", gtk_callback(&SomeClass::on_custom1_drag_drop) },
    { "custom1", "drag_end", gtk_callback(&SomeClass::on_custom1_drag_end) },
    { "custom1", "drag_leave", gtk_callback(&SomeClass::on_custom1_drag_leave) },
    { "custom1", "drag_motion", gtk_callback(&SomeClass::on_custom1_drag_motion) },
    { "custom1", "draw", gtk_callback(&SomeClass::on_custom1_draw) },
    { "custom1", "draw_default", gtk_callback(&SomeClass::on_custom1_draw_default) },
    { "custom1", "draw_focus", gtk_callback(&SomeClass::on_custom1_draw_focus) },
    { "custom1", "enter_notify_event", gtk_callback(&SomeClass::on_custom1_enter_notify_event) },
    { "custom1", "event", gtk_callback(&SomeClass::on_custom1_event) },
    { "custom1", "expose_event", gtk_callback(&SomeClass::on_custom1_expose_event) },
    { "custom1", "focus_in_event", gtk_callback(&SomeClass::on_custom1_focus_in_event) },
    { "custom1", "grab_focus", gtk_callback(&SomeClass::on_custom1_grab_focus ) },
    { "custom1", "selection_request_event", gtk_callback(&SomeClass::on_custom1_selection_request_event) },
    { "custom1", "show", gtk_callback(&SomeClass::on_custom1_show) },
    { "custom1", "size_allocate", gtk_callback(&SomeClass::on_custom1_size_allocate) },
    { "custom1", "size_request", gtk_callback(&SomeClass::on_custom1_size_request) },
    { "custom1", "state_changed", gtk_callback(&SomeClass::on_custom1_state_changed) },
    { "custom1", "style_set", gtk_callback(&SomeClass::on_custom1_style_set) },
    { "custom1", "unmap", gtk_callback(&SomeClass::on_custom1_unmap) },
    { "custom1", "unmap_event", gtk_callback(&SomeClass::on_custom1_unmap_event) },
    { "custom1", "unrealize", gtk_callback(&SomeClass::on_custom1_unrealize) },
    { "custom1", "visibility_notify_event", gtk_callback(&SomeClass::on_custom1_visibility_notify_event) },
    { "custom1", "destroy", gtk_callback(&SomeClass::on_custom1_destroy) },
    { }
  };

  SomeClass sc;

  gtk_init(0, NULL);
  GtkGladeWindow win;

  win.readGladeFile("example.glade", "window1", &sc, table);

  win.show();

  testv2();
};
