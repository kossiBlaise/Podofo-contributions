TEMPLATE = app
LANGUAGE = C++

#CONFIG += STATIC_
CONFIG += PODOFO_

DESTDIR = ./
OBJECTS_DIR = ./Obj
MOC_DIR = ./moc

year = 2019

QT += gui widgets
CONFIG   += warn_on release
win32:CONFIG   += console


!STATIC_ {
   PODOFO_ {
      PROG_BASE = p:/podofo-1-0-1/podofoLib/$$year-x86
   }
   !PODOFO_ {
      PROG_BASE = e:/vcpkg/installed/x86-windows
   }
   AUXLIB_ = e:/vcpkg/installed/x86-windows/lib
}

INCLUDEPATH += $$PROG_BASE/include

QMAKE_LIBDIR += $$PROG_BASE/LIB

LIBS  += kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib
LIBS  += shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib Crypt32.lib
LIBS  += rpcrt4.lib oldnames.lib

LIBS  += podofo.lib

PODOFO_ {
   DEFINES += PODOFO_
}

# PRE_TARGETDEPS += qpodofo.h

HEADERS  += pdfPageLabel.h
TARGET = mx
SOURCES  += main.cpp
SOURCES += pdfPageLabel.cpp
HEADERS += pdfPageLabel.h



#message($$PODOFO_BASE)
#message($(PWD))
FORMS +=
QMAKE_CXXFLAGS += -std:c++17      # make sure we are using c17 compatable environment
# message("ProgBase: " $$PROG_BASE)