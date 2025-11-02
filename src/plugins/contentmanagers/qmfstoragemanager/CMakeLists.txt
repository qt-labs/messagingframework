add_library(qmfstoragemanager MODULE qmfstoragemanager.h qmfstoragemanager.cpp)
target_include_directories(qmfstoragemanager
  PRIVATE ../../../libraries/qmfclient ../../../libraries/qmfclient/support)
target_link_libraries(qmfstoragemanager
  PRIVATE Qt6::Core QmfClient)

install(TARGETS qmfstoragemanager
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/messagingframework/contentmanagers)

if(BUILD_TESTING)
  add_custom_target(link_qmfstoragemanager ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink qmfstoragemanager/libqmfstoragemanager.so ../libqmfstoragemanager.so
    DEPENDS qmfstoragemanager)
endif()
