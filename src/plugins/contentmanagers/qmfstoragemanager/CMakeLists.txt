add_library(qmfstoragemanager MODULE qmfstoragemanager.h qmfstoragemanager.cpp)
target_include_directories(qmfstoragemanager
  PRIVATE ../../../libraries/qmfclient ../../../libraries/qmfclient/support)
target_link_libraries(qmfstoragemanager
  PRIVATE Qt6::Core QmfClient)

install(TARGETS qmfstoragemanager
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt5/plugins/contentmanagers)
