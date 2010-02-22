CONFIG(debug,debug|release) {
    DEFINES += QMF_ENABLE_LOGGING
}

win32 {

    RELEASEMODE=unspecified

    !build_pass {

        contains(CONFIG_WIN,debug) {
            RELEASEMODE=debug
        } else:contains(CONFIG_WIN,release) {
            RELEASEMODE=release
        }

        # In Windows we want to build libraries in debug and release mode if the user
        # didn't select a version, and if Qt is built in debug_and_release.
        # This avoids problems for third parties as qmake builds debug mode by default
        # Silently disable unsupported configurations

        CONFIG -= debug release debug_and_release build_all

        contains(RELEASEMODE,unspecified) {
            contains(QT_CONFIG,debug):contains(QT_CONFIG,release) {
                CONFIG += debug_and_release build_all
            } else {
                contains(QT_CONFIG,debug): CONFIG+=debug
                contains(QT_CONFIG,release): CONFIG+=release
            }
        } else {
            CONFIG += $$RELEASEMODE
        }
    }

    # If we are building both debug and release configuration, ensure
    # debug binaries load debug versions of their dependents

    CONFIG(debug,debug|release) {
        DEFINES += LOAD_DEBUG_VERSION
    }

    #suffix changes

    contains(TEMPLATE,.*lib) {
        TARGET=$$qtLibraryTarget($${TARGET})
    } 

    contains(TEMPLATE,.*app) {
        CONFIG(debug,debug|release):TARGET=$$join(TARGET,,,d)
    }

}

INSTALLS += target

DESTDIR=build

# build frameworks on mac

mac:contains(QT_CONFIG,qt_framework) {
    CONFIG += lib_bundle absolute_library_soname
    FRAMEWORK_HEADERS.version = Versions
    FRAMEWORK_HEADERS.files = $${PUBLIC_HEADERS}
    FRAMEWORK_HEADERS.path = Headers
    QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
}

qtopiamail:qtAddLibrary(qtopiamail)
messageserver:qtAddLibrary(messageserver)
qmfutil:qtAddLibrary(qmfutil)


