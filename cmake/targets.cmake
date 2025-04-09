## version management

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.h
    COMMAND awk -F'[.-]' '{ printf \"\#define VERSION \\"%d.%d.%d-%d\\"\\n\" , $$1, $$2, $$3, $$4 > \"${CMAKE_CURRENT_BINARY_DIR}/version.h\"}' ${CMAKE_CURRENT_SOURCE_DIR}/version
)

add_custom_target(
	"version"
	COMMAND awk -F'[.-]' '{ printf \"%d.%d.%d-%d\" , $$1, $$2, $$3, $$4+1 > \"${CMAKE_CURRENT_SOURCE_DIR}/version\"}' ${CMAKE_CURRENT_SOURCE_DIR}/version
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_dependencies(${PROJECT_NAME} version)
#

add_custom_target("bump" COMMENT "bumping minor version")
add_custom_command(
    TARGET "bump"
    COMMENT "bumping minor version"
	COMMAND awk -F'[.-]' '{ printf \"%d.%d.%d-%d\" , $$1, $$2, $$3+1, $$4 > \"${CMAKE_CURRENT_SOURCE_DIR}/version\"}' ${CMAKE_CURRENT_SOURCE_DIR}/version
	COMMAND awk -F'[.-]' '{ printf \"\#define VERSION \\"%d.%d.%d-%d\\"\\n\" , $$1, $$2, $$3, $$4 > \"${CMAKE_CURRENT_BINARY_DIR}/version.h\"}' ${CMAKE_CURRENT_SOURCE_DIR}/version
)

##

## make release
add_custom_target(release)
add_custom_command(TARGET release COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/version.h)
add_dependencies(release bump version ${PROJECT_NAME})

## make uninstall
add_custom_target("uninstall" COMMENT "Uninstall installed files")
add_custom_command(
	TARGET "uninstall"
	POST_BUILD
	COMMENT "Uninstall files with install_manifest.txt"
	COMMAND xargs rm -vf < install_manifest.txt || echo Nothing in install_manifest.txt to be uninstalled!
)

##
