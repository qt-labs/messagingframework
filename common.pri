CONFIG -= debug_and_release

CONFIG(debug,debug|release) {
    DEFINES += QMF_ENABLE_LOGGING
}
