##############################################################################
##############################################################################

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR RPM)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})

set(CPACK_PACKAGE_RELEASE "1")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "Amirul ISLAM")
set(CPACK_PACKAGE_VENDOR "binutil")
set(CPACK_PACKAGE_URL "https://github.com/thegeekyboy/axon")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")

set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RPM_PACKAGE_RELOCATABLE FALSE)
set(CPACK_RPM_PACKAGE_AUTOREQPROV " no")

#set(CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.spec")

configure_file(extras/${PROJECT_NAME}.spec.in ${PROJECT_NAME}.spec @ONLY IMMEDIATE)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/extras/scripts/pre-install.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/extras/scripts/pre-install.sh" @ONLY IMMEDIATE)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/extras/scripts/post-install.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/extras/scripts/post-install.sh" @ONLY IMMEDIATE)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/extras/scripts/pre-uninstall.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/extras/scripts/pre-uninstall.sh" @ONLY IMMEDIATE)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/extras/scripts/post-uninstall.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/extras/scripts/post-uninstall.sh" @ONLY IMMEDIATE)

set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/extras/scripts/pre-install.sh")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/extras/scripts/post-install.sh")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/extras/scripts/pre-uninstall.sh")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/extras/scripts/post-uninstall.sh")

include(CPack)
