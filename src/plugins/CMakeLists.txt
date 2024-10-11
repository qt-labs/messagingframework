add_subdirectory(contentmanagers)
add_subdirectory(messageservices)

find_program(GPGME_CONFIG gpgme-config)
if (GPGME_CONFIG)
  execute_process(COMMAND ${GPGME_CONFIG} --libs OUTPUT_VARIABLE GPGME_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
  add_subdirectory(crypto)
endif()
