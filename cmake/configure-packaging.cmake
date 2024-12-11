set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_COMPONENTS_ALL "ultra-viewer")

if (CPACK_GENERATOR STREQUAL "External")
  list(APPEND CPACK_COMPONENTS_ALL "extractor" "appimage")
endif()

if (CPACK_GENERATOR MATCHES "DEB|RPM")
# https://unix.stackexchange.com/a/11552/254512
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/ship/bin")#/${CMAKE_PROJECT_VERSION}")
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY 0)
elseif (CPACK_GENERATOR MATCHES "ZIP")
set(CPACK_PACKAGING_INSTALL_PREFIX "")
endif()

if (CPACK_GENERATOR MATCHES "External")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
SET(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/bin")
endif()

if (CPACK_GENERATOR MATCHES "Bundle")
    set(CPACK_BUNDLE_NAME "ultra-viewer")
    add_custom_target(CreateOSXIcons
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset
    COMMAND sips -z 16 16     logo.png --out ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_16x16.png
    COMMAND sips -z 32 32     logo.png --out ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_16x16@2x.png
    COMMAND sips -z 32 32     logo.png --out ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_32x32.png
    COMMAND sips -z 64 64     logo.png --out ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_32x32@2x.png
    COMMAND sips -z 128 128   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_128x128.png
    COMMAND sips -z 256 256   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_128x128@2x.png
    COMMAND sips -z 256 256   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_256x256.png
    COMMAND sips -z 512 512   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_256x256@2x.png
    COMMAND sips -z 512 512   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_512x512.png
    COMMAND cp                logo.png ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset/icon_512x512@2x.png
    COMMAND iconutil -c icns -o ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.icns ${CMAKE_BINARY_DIR}/macosx/ultra-viewer.iconset
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Creating OSX icons ..."
    )
    add_dependencies(${PROJECT_NAME} CreateOSXIcons)
    # set(CPACK_BUNDLE_PLIST "macosx/Info.plist")
    set(CPACK_BUNDLE_ICON "macosx/ultra-viewer.icns")
    # set(CPACK_BUNDLE_STARTUP_COMMAND "macosx/Starship-macos.sh")
    set(CPACK_BUNDLE_APPLE_CERT_APP "-")
endif()