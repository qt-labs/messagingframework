add_executable(tst_crypto tst_crypto.cpp)
target_include_directories(tst_crypto
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_crypto
  PRIVATE Qt6::Core Qt6::Test QmfClient ${GPGME_LIBS})

add_test(NAME tst_crypto COMMAND tst_crypto)
set_tests_properties(tst_crypto PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_crypto
  RUNTIME DESTINATION /opt/tests/qmf)
