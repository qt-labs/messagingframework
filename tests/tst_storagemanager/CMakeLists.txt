add_executable(tst_storagemanager tst_storagemanager.cpp)
target_include_directories(tst_storagemanager
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_storagemanager
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_storagemanager
  RUNTIME DESTINATION /opt/tests/qmf)
