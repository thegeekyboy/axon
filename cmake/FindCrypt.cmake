# Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
# This file is part of Rozofs.
#
# Rozofs is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 2.
#
# Rozofs is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

# - Find crypt
# Find the native CRYPT includes and library
#
#  CRYPT_INCLUDE_DIRS - where to find crypt.h, etc.
#  CRYPT_LIBRARIES   - List of libraries when using crypt.
#  CRYPT_FOUND       - True if crypt found.

FIND_PATH(CRYPT_INCLUDE_DIRS crypt.h
  /usr/local/include/crypt
  /usr/local/include
  /usr/include/crypt
  /usr/include
)

SET(CRYPT_NAMES crypt)
FIND_LIBRARY(CRYPT_LIBRARYS
  NAMES ${CRYPT_NAMES}
  PATHS /usr/lib /usr/lib64 /usr/local/lib
)

IF(CRYPT_INCLUDE_DIRS AND CRYPT_LIBRARYS)
  SET(CRYPT_FOUND TRUE)
  SET(CRYPT_LIBRARIES ${CRYPT_LIBRARY})
  MESSAGE(STATUS "Found Crypt at ${CRYPT_LIBRARYS}")
ELSE(CRYPT_INCLUDE_DIRS AND CRYPT_LIBRARYS)
  SET(CRYPT_FOUND FALSE)
  SET(CRYPT_LIBRARIES)
ENDIF(CRYPT_INCLUDE_DIRS AND CRYPT_LIBRARYS)

IF(NOT CRYPT_FOUND)
   IF(CRYPT_FIND_REQUIRED)
     MESSAGE(FATAL_ERROR "crypt library and headers required.")
   ENDIF(CRYPT_FIND_REQUIRED)
ENDIF(NOT CRYPT_FOUND)

MARK_AS_ADVANCED(
  CRYPT_LIBRARYS
  CRYPT_INCLUDE_DIRS
)