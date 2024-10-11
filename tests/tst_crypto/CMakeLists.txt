add_executable(tst_crypto tst_crypto.cpp)
target_include_directories(tst_crypto
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_crypto
  PRIVATE Qt6::Core Qt6::Test QmfClient ${GPGME_LIBS})

install(TARGETS tst_crypto
  RUNTIME DESTINATION /opt/tests/qmf)
