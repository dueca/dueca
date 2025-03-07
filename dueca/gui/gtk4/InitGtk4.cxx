/* ------------------------------------------------------------------   */
/*      item            : InitGtk4.cxx
        made by         : Rene' van Paassen
        date            : ???
        category        : Body, init file
        description     :
        changes         : ??????? first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#include <scriptinterface.h>
#include <dueca/visibility.h>
#include "ActivityView.hxx"
#include "GtkDuecaView.hxx"
#include "ChannelOverviewGtk4.hxx"
#include "TimingViewGtk.hxx"
#include "GtkHandler.hxx"
#include "LogView.hxx"
#include "DuecaEnv.hxx"
#include <string>
#include <iostream>
#include <StartIOStream.hxx>
#include <iostream>

#define DO_INSTANTIATE
#include "TypeCreator.hxx"

DUECA_NS_START

extern "C"
LNK_PUBLICC void InitGtk4()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-gtk4]" << std::endl;
  }
  static TypeCreator<TimingViewGtk> t05(TimingView::getParameterTable());
  static TypeCreator<ActivityView>  t01(ActivityView::getParameterTable());
  static TypeCreator<GtkDuecaView>  t02(GtkDuecaView::getParameterTable());
  static TypeCreator<LogView>       t03(LogView::getParameterTable());
  static TypeCreator<ChannelOverviewGtk4>
    t04(ChannelOverviewGtk4::getMyParameterTable());
  static GtkHandler h1(std::string("gtk4"));
}


DUECA_NS_END
