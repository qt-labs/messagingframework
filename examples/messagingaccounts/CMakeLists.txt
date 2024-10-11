qt_add_resources(SRC messagingaccounts.qrc)

add_executable(messagingaccounts5 ${SRC} main_messagingaccounts.cpp)
target_include_directories(messagingaccounts5 PRIVATE ../qtmail)
target_link_libraries(messagingaccounts5
  PUBLIC Qt6::Core QtMail)
