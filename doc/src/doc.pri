QDOC_BIN = $$targetPath($$[QT_INSTALL_BINS]/qdoc3)
HELPGENERATOR = $$targetPath($$[QT_INSTALL_BINS]/qhelpgenerator)

equals(QMAKE_DIR_SEP, /) {   # unix, mingw+msys
    QDOC = SRCDIR=$$PWD OUTDIR=$$OUT_PWD/doc/html $$QDOC_BIN
} else:win32-g++* {   # just mingw
    # The lack of spaces in front of the && is necessary!
    QDOC = set SRCDIR=$$PWD&& set OUTDIR=$$OUT_PWD/doc/html&& $$QDOC_BIN
} else {   # nmake
    QDOC = set SRCDIR=$$PWD $$escape_expand(\\n\\t) \
           set OUTDIR=$$OUT_PWD/doc/html $$escape_expand(\\n\\t) \
           $$QDOC_BIN
}

QHP_FILE = $$OUT_PWD/doc/html/qmf.qhp
QCH_FILE = $$OUT_PWD/doc/html/qmf.qch

HELP_DEP_FILES = $$PWD/index.qdoc \
                 $$PWD/messageserver.qdoc \
                 $$PWD/messaging.qdoc \
                 $$PWD/qtmail.qdoc \
                 $$PWD/qtopiamail.qdoc \
                 $$PWD/qtopiamail_messageserver.qdoc \
                 $$PWD/qtopiamail_qmfutil.qdoc \
                 $$PWD/qmf.qdocconf \

html_docs.commands = $$QDOC $$PWD/qmf.qdocconf
html_docs.depends += $$HELP_DEP_FILES
html_docs.files = $$QHP_FILE

qch_docs.commands = $$HELPGENERATOR -o \"$$QCH_FILE\" $$QHP_FILE
qch_docs.depends += html_docs
qch_docs.files = $$QCH_FILE

unix:!macx {
    qch_docs.path = $$[QT_INSTALL_PREFIX]/share/qmf/doc
    qch_docs.CONFIG += no_check_exist
    INSTALLS += qch_docs
}

docs.depends = qch_docs
QMAKE_EXTRA_TARGETS += html_docs qch_docs docs

OTHER_FILES = $$HELP_DEP_FILES \
              $$PWD/api/api-pages.qdoc \
              $$PWD/api/classhierarchy.qdoc \
              $$PWD/api/groups.qdoc \
              $$PWD/examples/messageviewer.qdoc \
              $$PWD/examples/messagenavigator.qdoc
