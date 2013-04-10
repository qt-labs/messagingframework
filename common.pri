CONFIG(debug,debug|release) {
    DEFINES += QMF_ENABLE_LOGGING
}


win32 | macx {

    RELEASEMODE=unspecified

    !build_pass {

        win32 {
            contains(CONFIG_WIN,debug) {
                RELEASEMODE=debug
            } else:contains(CONFIG_WIN,release) {
                RELEASEMODE=release
            }
        }

        # In Windows we want to build libraries in debug and release mode if the user
        # didn't select a version, and if Qt is built in debug_and_release.
        # This avoids problems for third parties as qmake builds debug mode by default
        # Silently disable unsupported configurations

        # MacOSX always builds debug and release libs when using mac framework

        CONFIG -= debug release debug_and_release build_all

        contains(RELEASEMODE,unspecified) {
            contains(QT_CONFIG,debug):contains(QT_CONFIG,release) | if(macx:contains(QT_CONFIG,qt_framework):contains(TEMPLATE,.*lib)) {
                CONFIG += debug_and_release build_all
            } else {
                contains(QT_CONFIG,debug): CONFIG+=debug
                contains(QT_CONFIG,release): CONFIG+=release
            }
        } else {
            CONFIG += $$RELEASEMODE
        }
    }

    

    #suffix changes

    contains(TEMPLATE,.*lib) {
        TARGET=$$qtLibraryTarget($${TARGET})
    } 

    win32 {
        contains(TEMPLATE,.*app) {
            CONFIG(debug,debug|release):TARGET=$$join(TARGET,,,d)
        }

        # If we are building both debug and release configuration, ensure
        # debug binaries load debug versions of their dependents

        CONFIG(debug,debug|release) {
            DEFINES += LOAD_DEBUG_VERSION
        }
    }

}

INSTALLS += target

DESTDIR=build

plugin {
    DEFINES += PLUGIN_INTERNAL
}

# build QMF libraries as frameworks on OSX, omitting plugins.

mac:contains(QT_CONFIG,qt_framework):!plugin {
    CONFIG += lib_bundle absolute_library_soname
    FRAMEWORK_HEADERS.version = Versions
    FRAMEWORK_HEADERS.files = $${PUBLIC_HEADERS}
    FRAMEWORK_HEADERS.path = Headers
    QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
}

CONFIG += hide_symbols
