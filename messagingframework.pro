
requires(!wince)
requires(!winrt)
requires(!ios)
requires(!qnx)
requires(!android)
requires(!integrity)
requires(!vxworks)
requires(!wasm)
requires(!macos)

requires(qtHaveModule(sql))

!contains(DEFINES,QMF_NO_WIDGETS) {
    requires(qtHaveModule(widgets))
}

load(qt_parts)

!unix {
     warning("IMAP COMPRESS capability is currently not supported on non unix platforms")
}
