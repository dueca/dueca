# -------------------------------------------------------------------
#       item            : udpcom/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 241230
#       category        : cmake definition
#       copyright       : (c) 2024 Rene van Paassen

if(BUILD_GTK4)
  add_subdirectory(gtk4)
endif()
if(BUILD_GTK3)
  add_subdirectory(gtk3)
endif()
if(BUILD_GTK2)
  add_subdirectory(gtk2)
endif()
