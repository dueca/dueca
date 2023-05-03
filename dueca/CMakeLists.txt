# -------------------------------------------------------------------   */
#       item            : dueca/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

find_package(DuecaCodegen)
find_package(DuecaAddLibrary)
find_package(RapidJSON)

set(DCOSOURCES

  NameSet.dco EndRole.dco ChannelDistribution.dco DataTimeSpec.dco
  ModuleState.dco EntityUpdate.dco NodeControlMessage.dco
  ScriptLine.dco ActivityLogRequest.dco ActivityLogRequest.dco
  ObjectInfo.dco ScriptConfirm.dco FillSet.dco EntityCommand.dco
  EntityConfirm.dco TimingResults.dco SyncReport.dco
  ActivityDescription.dco SyncReportRequest.dco LogMessage.dco
  LogTime.dco LogPoint.dco SimStateRequest.dco LogLevelCommand.dco
  GlobalId.dco SnapshotState.dco SimulationState.dco LogLevel.dco
  ActivityLog.dco ChannelChangeNotification.dco ChannelEndUpdate.dco
  UChannelCommRequest.dco ChannelReadInfo.dco ChannelWriteInfo.dco
  TokenCountResult.dco EntryCountResult.dco ChannelCountResult.dco
  ChannelCountRequest.dco ChannelMonitorResult.dco
  ChannelMonitorRequest.dco )

set(DUECASOURCES

  Packer.cxx Packer.hxx Unpacker.cxx Unpacker.hxx FillPacker.cxx
  FillPacker.hxx FillUnpacker.cxx FillUnpacker.hxx Activity.cxx
  Activity.hxx ActivityBit.cxx ActivityBit.hxx ActivityLister.cxx
  ActivityLister.hxx ActivityManager.cxx ActivityManager.hxx
  ActivityWeaver.cxx ActivityWeaver.hxx AmorphStore.cxx
  AmorphStore.hxx Arena.cxx Arena.hxx ArenaPool.cxx ArenaPool.hxx
  ChannelIdList.cxx ChannelManager.cxx ChannelOrganiser.cxx
  ClockTime.cxx Condition.cxx DataSetConverter.cxx
  DataSetSubsidiary.hxx Dstring.cxx EntityManager.cxx EntityState.cxx
  Environment.cxx Event.hxx EventAccessToken.hxx Exception.cxx
  GenericCallback.cxx GenericEvent.cxx GenericPacker.cxx
  GenericToken.cxx GuileStart.hxx CyclicInt.cxx MemberCall.hxx
  Module.cxx ModuleIdList.cxx NamedChannel.cxx NamedObject.cxx
  NodeManager.cxx ObjectManager.cxx ParameterTable.cxx
  PrioritySpec.cxx ScriptInterpret.cxx SimTime.cxx StateChange.hxx
  StateGuard.cxx StreamAccessToken.hxx Ticker.cxx TimeKeeper.cxx
  TimeSpec.cxx TransportClass.cxx Trigger.cxx TriggerAtom.cxx
  VarProbe.hxx registry.hxx GuiHandler.cxx EventReader.hxx
  StreamReader.hxx StreamReaderLatest.hxx StreamWriter.hxx Su.cxx
  StoreInformation.cxx ReflectivePacker.cxx ReflectiveUnpacker.cxx
  ReflectiveAccessor.cxx Entity.cxx TransportNotification.cxx
  CriticalActivity.cxx ReflectiveFillPacker.cxx
  ReflectiveFillUnpacker.cxx DuecaPath.cxx Summary.cxx ModuleId.cxx
  StatusKeeper.cxx StatusT1.cxx TimingCheck.cxx Accessor.cxx
  TimeWarp.cxx ScriptCreatable.cxx MemberCall2Way.hxx
  AperiodicAlarm.cxx PeriodicAlarm.cxx DuecaEnv.cxx AsyncList.cxx
  AsyncList.hxx DuecaView.cxx LogCategory.cxx LogConcentrator.cxx
  Logger.cxx newlog.cxx newlog-macros.hxx LogString.cxx
  ThreadSpecific.cxx ThreadSpecific.hxx ActivityContext.cxx
  PackerManager.cxx PackerManager.hxx ActivityDescriptions.cxx
  ActivityDescriptions.hxx LogPoints.cxx debug-direct.cxx
  WindowingProtocol.cxx DataClassRegistry.cxx DataClassRegistrar.cxx
  CommObjectMemberAccess.hxx CommObjectReader.cxx CommObjectWriter.cxx
  CommObjectTraits.cxx CommObjectExceptions.cxx UnifiedChannel.cxx
  UChannelEntry.cxx UChannelEntryData.cxx ChannelReadToken.cxx
  ChannelReadToken.hxx ChannelWriteToken.cxx UnifiedChannelMaster.cxx
  UCClientHandle.cxx AsyncQueueMT.hxx UCEntryDataCache.cxx
  UCEntryDataCache.hxx CommObjectReaderWriter.cxx ChannelWatcher.cxx
  trace.c StartIOStream.cxx TriggerRegulator.cxx GenericVarIO.cxx
  TriggerRegulatorGreedy.cxx TimedServicer.cxx MessageBuffer.cxx
  MessageBuffer.hxx ChannelDef.hxx ChannelDef.cxx vectorMT.hxx
  DCOFunctor.hxx DCOFunctor.cxx DCOMetaFunctor.hxx DCOMetaFunctor.cxx
  ChannelEntryInfo.hxx ChannelEntryInfo.cxx ActivityViewBase.cxx
  ActivityViewBase.hxx ScriptHelper.hxx ScriptHelper.cxx
  ModuleCreator.cxx TimingView.cxx TimingView.hxx PackerSet.cxx
  CreationCenter.cxx GenericTypeCreator.cxx ArgElement.cxx
  DuecaMain.cxx DCOtoJSON.hxx DCOtoJSON.cxx JSONtoDCO.hxx
  JSONtoDCO.cxx ChannelOverview.cxx ChannelOverview.hxx
  ChannelDataMonitor.cxx ChannelDataMonitor.hxx CPULowLatency.cxx
  CPULowLatency.hxx CommonCallback.hxx smartstring.hxx smartstring.cxx
  XMLtoDCO.hxx XMLtoDCO.cxx DCOtoXML.hxx DCOtoXML.cxx
  DataWriterArraySize.hxx msgpack.hxx msgpack.cxx
  PythonCorrectedName.hxx PythonCorrectedName.cxx
  ListElementAllocator.hxx DCOtypeJSON.hxx DCOtypeJSON.cxx
  ChronoTimePoint.hxx ChronoTimePoint.cxx AssociateObject.hxx
  EasyId.cxx EasyId.hxx InformationStash.cxx)


set(SCRAMNETSOURCES

  ScramNetAccessor.cxx InitScram.cxx)

set(IPSOURCES

  IPBroadcastAccessor.cxx
  IPMulticastAccessor.cxx IPTwoWay.cxx IPAccessor.cxx
  TransportDelayEstimator.cxx)

set(SHMSOURCES
  ShmAccessor.cxx)

set(STARTSOURCESSCHEME

  ArgListProcessor.cxx InitScheme.cxx SchemeObject.cxx
  CreationCenter_Scheme.hxx Entity_Scheme.hxx
  CreationCenter.hxx SchemeClassData.hxx
  SchemeClassData.cxx  TypeCreator.cxx
  SchemeScripting.cxx
  SchemeScripting.hxx CoreCreator.cxx dueca_assert.c)

set(STARTSOURCESPYTHON

  ArgListProcessor.cxx InitPython.cxx
  CreationCenter_Python.hxx TypeCreator.cxx
  PythonScripting.cxx PythonScripting.hxx)

set(CONFSOURCES includeguile.h nodes.h debug.h
  stringoptions.h dueca-version.h DAtomics.hxx DCOTypeIndex.hxx)

set(INSTALLHEADERS

  Activity.hxx AmorphStore.hxx Arena.hxx ArenaPool.hxx Callback.hxx
  Callback.ixx CallbackWithId.hxx ClockTime.hxx Condition.hxx
  DataSetConverter.hxx DataSetSubsidiary.hxx visibility.h Dstring.hxx
  Entity.hxx EntityState.hxx Environment.hxx Event.hxx
  EventAccessToken.hxx Exception.hxx GenericCallback.hxx
  GenericEvent.hxx GenericToken.hxx GenericVarIO.hxx MemberCall.hxx
  Module.hxx ModuleCreator.hxx NamedChannel.hxx NamedObject.hxx
  ParameterTable.hxx PrioritySpec.hxx SimTime.hxx StateChange.hxx
  GenericTypeCreator.hxx StateGuard.hxx StreamAccessToken.hxx
  Ticker.hxx TimeSpec.hxx WrapSendEvent.hxx TransportClass.hxx
  Trigger.hxx TypeCreator.hxx VarProbe.hxx all_exceptions.h dstypes.h
  dueca.h dueca_ns.h EventReader.hxx StreamReader.hxx StreamWriter.hxx
  Su.hxx StreamReaderLatest.hxx CriticalActivity.hxx GuileStart.hxx
  AsyncList.hxx MessageBuffer.hxx scriptinterface.h ArgElement.hxx
  AsyncQueueMT.hxx LockFreeLIFO.hxx EventWriter.hxx
  SharedPtrTemplates.hxx SchemeObject.hxx ScriptInterpret.hxx
  ChannelEntryInfo.hxx SchemeData.hxx TimingCheck.hxx Exception.ixx
  dueca_assert.h StatusT1.hxx StatusKeeper.hxx NodeManager.hxx
  EntityManager.hxx Summary.hxx ModuleId.hxx TimeWarp.hxx
  dueca-guile.h ScriptCreatable.hxx MemberCall2Way.hxx CoreCreator.hxx
  CoreCreator.ixx ArgListProcessor.hxx AperiodicAlarm.hxx
  PeriodicAlarm.hxx DuecaEnv.hxx MultiStreamReadToken.hxx
  MultiStreamWriteToken.hxx MultiStreamWriter.hxx
  MultiStreamReader.hxx Logger.hxx LogCategory.hxx newlog.hxx
  newlog-macros.hxx LogString.hxx ActivityContext.hxx debug-direct.h
  ScriptCreatableDataHolder.hxx PackUnpackTemplates.hxx fixvector.hxx
  varvector.hxx limvector.hxx vectorexceptions.hxx
  DataClassRegistryPredef.hxx DataClassRegistry.hxx
  DataClassRegistrar.hxx CommObjectMemberAccess.hxx NameSetExtra.hxx
  ModuleStateExtra.hxx GlobalIdExtra.hxx SimulationStateExtra.hxx
  DataTimeSpecExtra.hxx LogLevelExtra.hxx ChannelDistributionExtra.hxx
  EndRoleExtra.hxx ActivityLogExtra.hxx CommObjectReader.hxx
  CommObjectWriter.hxx CommObjectTraits.hxx PackTraits.hxx
  CommObjectElementReader.hxx CommObjectElementWriter.hxx
  CommObjectElementWriterBase.hxx CommObjectElementReaderBase.hxx
  CommObjectExceptions.hxx ChannelReadInfoExtra.hxx
  ChannelWriteInfoExtra.hxx ChannelReadToken.hxx DataReader.hxx
  DataReaderBase.hxx ChannelWriteToken.hxx DataWriter.hxx
  DataUpdater.hxx UChannelCommRequestExtra.hxx TimedServicer.hxx
  ChannelDef.hxx CommObjectMemberArity.hxx CommObjectReaderWriter.hxx
  ChannelWatcher.hxx StartIOStream.hxx TriggerRegulator.hxx
  TriggerRegulatorGreedy.hxx DCOFunctor.hxx DCOMetaFunctor.hxx
  SchemeClassData.hxx CoreCreatorPython.ixx DCOtoJSON.hxx
  JSONtoDCO.hxx CommonCallback.hxx XMLtoDCO.hxx DCOtoXML.hxx
  smartstring.hxx DataWriterArraySize.hxx msgpack.hxx
  msgpack-unstream.hxx PythonCorrectedName.hxx ChronoTimePoint.hxx
  msgpack-unstream-iter.hxx msgpack-unstream-iter.ixx
  ListElementAllocator.hxx DCOtypeJSON.hxx undebug.h
  AssociateObject.hxx EasyId.hxx)

DUECACODEGEN_TARGET(OUTPUT DCO INDUECA DCOSOURCE ${DCOSOURCES})

foreach(S ${CONFSOURCES})
  configure_file(${S}.in ${S} @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${S}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/dueca)
endforeach()
configure_file(DuecaPath.ixx.in DuecaPath.ixx @ONLY)

# handle the boost versions
configure_file(dueca-conf-intrusive.h.in dueca-conf-intrusive.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-conf-intrusive.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/dueca)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${PYTHON_INCLUDE_DIRS})

# xml help
pkg_check_modules(PUGIXML REQUIRED pugixml)

# common libraries
dueca_add_library(dueca
  SOURCES ${DCO_OUTPUT_HEADERS} ${DCO_OUTPUT_SOURCE} ${DUECASOURCES}
  LINKLIBS ${CMAKE_THREAD_LIBS_INIT} ${PUGIXML_LIBRARIES} ${ATOMIC_LIBRARY})

if (BUILD_SHM)
  dueca_add_library(dueca-shm
    SOURCES ${SHMSOURCES}
    LINKLIBS dueca)
endif()
if (BUILD_IP)
  dueca_add_library(dueca-ip
    SOURCES ${IPSOURCES}
    LINKLIBS dueca)
endif()


# start-script dependent libraries in the "start"
foreach(S ${SCRIPTLANG})

  dueca_add_library(dueca-start-${SCRIPT_${S}_SUFFIX} ${STARTLIB_STATIC}
    SOURCES ${STARTSOURCES${S}} ${DCO_OUTPUT_HEADERS}
    LINKLIBS ${SCRIPT_${S}_LIBS} dueca
    LINKDIRS ${SCRIPT_${S}_LINKDIRS}
    INCLUDEDIRS ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS "-DSCRIPT_${S}")
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  mylinker_arguments(OUTPUT SCRIPT_LDFLAGS LIBLIST ${SCRIPT_${S}_LIBS})
  set(SCRIPT_FLAGS "${SCRIPT_${S}_FLAGS}")
  configure_file(dueca-start-ln.pc.in
    dueca-start-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-start-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

  if (BUILD_SHM)
    dueca_add_library(dueca-shm-${SCRIPT_${S}_SUFFIX} ${STARTLIB_STATIC}
      SOURCES InitShm.cxx ${DCO_OUTPUT_HEADERS}
      LINKLIBS dueca-start-${SCRIPT_${S}_SUFFIX} dueca-shm
      INCLUDEDIRS ${SCRIPT_${S}_INCLUDES}
      COMPILEOPTIONS "-DSCRIPT_${S}")
    configure_file(dueca-shm-ln.pc.in
      dueca-shm-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
    install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/dueca-shm-${SCRIPT_${S}_SUFFIX}.pc
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
    if (SCRAMNET_FOUND)
      dueca_add_library(dueca-scramnet-${SCRIPT_${S}_SUFFIX} ${STARTLIB_STATIC}
        SOURCES ${SCRAMNETSOURCES}
        LINKLIBS dueca-shm-${SCRIPT_${S}_SUFFIX} ${SCRAMNET_LIBRARIES}
        INCLUDEDIRS ${SCRIPT_${S}_INCLUDES}
        COMPILEOPTIONS "-DSCRIPT_${S}")
      configure_file(dueca-scramnet-ln.pc.in
        dueca-scramnet-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
      install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/dueca-scramnet-${SCRIPT_${S}_SUFFIX}.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
   endif()
  endif()

  if (BUILD_IP)
    dueca_add_library(dueca-ip-${SCRIPT_${S}_SUFFIX} ${STARTLIB_STATIC}
      SOURCES InitIp.cxx ${DCO_OUTPUT_HEADERS} 
      LINKLIBS dueca-start-${SCRIPT_${S}_SUFFIX} dueca-ip
      INCLUDEDIRS ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
      ${SCRIPT_${S}_INCLUDES}
      COMPILEOPTIONS "-DSCRIPT_${S}")
    configure_file(dueca-ip-ln.pc.in
      dueca-ip-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
    install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/dueca-ip-${SCRIPT_${S}_SUFFIX}.pc
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  endif()

endforeach()

set(PKGCONFIGS

  dueca.pc dueca-ip.pc dueca-shm.pc dueca-scramnet.pc)

foreach(PKG ${PKGCONFIGS})
  configure_file(${PKG}.in ${PKG} @ONLY)
endforeach()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if (BUILD_SHM)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-shm.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

  if (SCRAMNET_FOUND)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-scramnet.pc
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  endif()
endif()

if (BUILD_IP)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-ip.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/dueca)

install(FILES ${DCO_OUTPUT_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/dueca)

add_subdirectory(gui)
