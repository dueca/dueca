/* ------------------------------------------------------------------   */
/*      item            : InitGtk2.cxx
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

#include "scriptinterface.h"
#include <dueca/visibility.h>
#include <cstdio>
#include "ActivityView.hxx"
#include "GtkDuecaView.hxx"
#include "ChannelOverviewGtk2.hxx"
#include "TimingViewGtk.hxx"
#include "GtkHandler.hxx"
#include "LogView.hxx"
#include <string>
#include <StartIOStream.hxx>
#include <iostream>
#include "DuecaEnv.hxx"

#define DO_INSTANTIATE
#include "TypeCreator.hxx"

DUECA_NS_START



extern "C"
LNK_PUBLICC void InitGtk2()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-gtk2]" << std::endl;
  }
  static TypeCreator<TimingViewGtk> t05(TimingView::getParameterTable());
  static TypeCreator<ActivityView>  t01(ActivityView::getParameterTable());
  static TypeCreator<GtkDuecaView>  t02(GtkDuecaView::getParameterTable());
  static TypeCreator<LogView>       t03(LogView::getParameterTable());
  static TypeCreator<ChannelOverviewGtk2>
    t04(ChannelOverviewGtk2::getMyParameterTable());
  static GtkHandler h1(std::string("gtk2"));
}


DUECA_NS_END
