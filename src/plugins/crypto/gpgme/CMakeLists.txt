add_library(gpgme MODULE
  ../common/qgpgme.h ../common/qgpgme.cpp
  gpgmeplugin.h gpgmeplugin.cpp)
target_include_directories(gpgme
  PRIVATE ../common ../../../libraries/qmfclient ../../../libraries/qmfclient/support)
target_link_libraries(gpgme
  PRIVATE Qt6::Core QmfClient ${GPGME_LIBS})

install(TARGETS gpgme
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt5/plugins/crypto)

if(BUILD_TESTING)
  add_custom_target(link_gpgme ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink gpgme/libgpgme.so ../libgpgme.so
    DEPENDS gpgme)
endif()
