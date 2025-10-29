qt_add_resources(SRC messagingaccounts.qrc)

add_executable(messagingaccounts ${SRC} main_messagingaccounts.cpp)
target_include_directories(messagingaccounts PRIVATE ../qtmail)
target_link_libraries(messagingaccounts
  PUBLIC Qt6::Core QtMail)
