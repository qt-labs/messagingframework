equals(QT_MAJOR_VERSION, 4): QDOC_BIN = $$targetPath($$[QT_INSTALL_BINS]/qdoc3)
equals(QT_MAJOR_VERSION, 5): QDOC_BIN = $$targetPath($$[QT_INSTALL_BINS]/qdoc)
HELPGENERATOR = $$targetPath($$[QT_INSTALL_BINS]/qhelpgenerator)

equals(QMAKE_DIR_SEP, /) {   # unix, mingw+msys
    QDOC = SRCDIR=$$PWD OUTDIR=$$_PRO_FILE_PWD_/doc/html $$QDOC_BIN
} else:win32-g++* {   # just mingw
    # The lack of spaces in front of the && is necessary!
    QDOC = set SRCDIR=$$PWD&& set OUTDIR=$$_PRO_FILE_PWD_/doc/html&& $$QDOC_BIN
} else {   # nmake
    QDOC = set SRCDIR=$$PWD $$escape_expand(\\n\\t) \
           set OUTDIR=$$_PRO_FILE_PWD_/doc/html $$escape_expand(\\n\\t) \
           $$QDOC_BIN
}

QHP_FILE = $$_PRO_FILE_PWD_/doc/html/qmf.qhp
QCH_FILE = $$_PRO_FILE_PWD_/doc/html/qmf.qch

HELP_DEP_FILES = $$_PRO_FILE_PWD_/doc/src/index.qdoc \
                 $$_PRO_FILE_PWD_/doc/src/messageserver.qdoc \
                 $$_PRO_FILE_PWD_/doc/src/messaging.qdoc \
                 $$_PRO_FILE_PWD_/doc/src/qtmail.qdoc \
                 $$_PRO_FILE_PWD_/doc/src/qtopiamail.qdoc \
                 $$_PRO_FILE_PWD_/doc/src/qtopiamail_messageserver.qdoc \
                 $$_PRO_FILE_PWD_/doc/src/qtopiamail_qmfutil.qdoc \
                 $$_PRO_FILE_PWD_/doc/src/qmf.qdocconf \

html_docs.commands = $$QDOC $$_PRO_FILE_PWD_/doc/src/qmf.qdocconf
html_docs.depends += $$HELP_DEP_FILES
html_docs.files = $$QHP_FILE

qch_docs.commands = $$HELPGENERATOR -o \"$$QCH_FILE\" $$QHP_FILE
qch_docs.depends += html_docs
qch_docs.files = $$QCH_FILE

unix:!macx {
    equals(QT_MAJOR_VERSION, 4): qch_docs.path = $$QMF_INSTALL_ROOT/share/doc/qmf/qch
    equals(QT_MAJOR_VERSION, 5): qch_docs.path = $$QMF_INSTALL_ROOT/share/doc/qmf-qt5/qch
    qch_docs.CONFIG += no_check_exist
}

docs.depends = qch_docs
QMAKE_EXTRA_TARGETS += html_docs qch_docs docs

OTHER_FILES = $$HELP_DEP_FILES \
              $$_PRO_FILE_PWD_/doc/src/api/api-pages.qdoc \
              $$_PRO_FILE_PWD_/doc/src/api/classhierarchy.qdoc \
              $$_PRO_FILE_PWD_/doc/src/api/groups.qdoc
