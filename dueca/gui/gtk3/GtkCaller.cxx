/* ------------------------------------------------------------------   */
/*      item            : GtkCaller.cxx
        made by         : Rene van Paassen
        date            : 051017
        category        : header file
        description     : This file provides template classes and
                          functions for interfacing with Gtk libglade
                          windows.
        api             :
        changes         : 051017 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define GtkCaller_cxx

#include <dueca-conf.h>
#include <dueca_ns.h>
#include "GtkCaller.hxx"

DUECA_NS_START

GtkCaller::GtkCaller() : gp(NULL) { }

GtkCaller::~GtkCaller() { }

void GtkCaller::setGPointer(gpointer g)
{
  gp = g;
}

DUECA_NS_END

