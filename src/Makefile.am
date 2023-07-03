lib_LTLIBRARIES = libaxon.la
VERINFO = -version-info 1:7744:0

libaxon_la_LDFLAGS = $(VERINFO) -no-undefined

libsrc = aes.cpp md5.cpp util.cpp dmi.cpp config.cpp log.cpp database.cpp socket.cpp connection.cpp ssh.cpp ftplist.cpp ftp.cpp mml.cpp message.cpp mail.cpp

libaxon_la_SOURCES = $(libsrc)

libcppflags = -D_REENTRANT

libaxon_la_CPPFLAGS = $(libcppflags)

include_HEADERS = $(libinc)

pkgconfigdir = $(libdir)/pkgconfig
