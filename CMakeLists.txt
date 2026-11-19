cmake_minimum_required(VERSION 3.25)

project(Mcenter VERSION 0.6.1 LANGUAGES CXX)
# release builds pass -DRELEASE_VERSION=<tag without v> so the app, About page and package names all agree with the git tag
if(RELEASE_VERSION)
    set(PROJECT_VERSION ${RELEASE_VERSION})
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Widgets
    LinguistTools
    DBus
    Network
)

qt_standard_project_setup()

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.ui
    src/mainwindow.cpp src/mainwindow.h
    src/operate.cpp src/operate.h
    src/powermonitor.cpp src/powermonitor.h
    src/helper.cpp src/helper.h
    src/msiwmi.cpp src/msiwmi.h
    src/biosupdate.cpp src/biosupdate.h
    src/companion.cpp src/companion.h
    src/msi-ec_helper.cpp src/msi-ec_helper.h
    src/settings.cpp src/settings.h
    src/dashboard.cpp src/dashboard.h src/theme.h
    src/sensors.cpp src/sensors.h
    src/pages.cpp src/pages.h src/uikit.h
    src/a11y.cpp src/a11y.h
    src/rgb.cpp src/rgb.h
    src/keyboard.cpp src/keyboard.h
    src/lighting.cpp
    src/ddc.cpp src/ddc.h src/displays.cpp src/displays.h
    src/appprofiles.cpp src/appprofiles.h
    src/bridge.cpp src/bridge.h
    src/resources.qrc
)

# Translations: the .ts sources live in src/i18n and resources.qrc embeds the compiled .qm files from the same folder, so compile them there
# (they are git-ignored build products). A clean checkout needs this to build at all.
set(TS_NAMES de_DE en es eu fi_FI fr hu it nb nl_NL pt_BR pt_PT ru tr vi zh_CN)
set(TS_FILES "")
foreach(n ${TS_NAMES})
    list(APPEND TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/i18n/Mcenter_${n}.ts)
endforeach()
foreach(ts ${TS_FILES})
    set_source_files_properties(${ts} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src/i18n)
endforeach()
qt_add_translation(QM_FILES ${TS_FILES})
add_custom_target(mcc_translations DEPENDS ${QM_FILES})

qt_add_executable(mcenter
    ${PROJECT_SOURCES}
)
add_dependencies(mcenter mcc_translations)

add_subdirectory(src/helper ${CMAKE_BINARY_DIR}/helper)

target_link_libraries(mcenter PRIVATE
    Qt6::Widgets
    Qt6::DBus
    Qt6::Network
)

include(GNUInstallDirs)
target_compile_definitions(mcenter PRIVATE
    Mcenter_VERSION="${PROJECT_VERSION}"
    MCC_DATADIR="/usr/share/mcenter")   # hardcoded like the other runtime paths this project looks up (doctor script, policies, ...): install.sh always installs under /usr regardless of CMAKE_INSTALL_PREFIX

# ───────────────────────────── install + packages (cmake --install, cpack -G DEB / RPM) ─────────────────────────────
# Mirrors install.sh. Build packages with:  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && cmake --build build && (cd build && cpack)
include(GNUInstallDirs)
install(TARGETS mcenter RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS scripts/mcenter-doctor scripts/mcenter-gpu scripts/mcenter-battery DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/mcenter)
install(FILES resources/mcenter.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/mcenter.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES resources/mcenter.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES src/helper/mcenter-helper.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d)
install(FILES src/helper/mcenter.helper.service DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services)
install(FILES packaging/io.github.p34t1k.mcenter.doctor.policy packaging/io.github.p34t1k.mcenter.gpu.policy packaging/io.github.p34t1k.mcenter.battery.policy packaging/io.github.p34t1k.mcenter.helper.policy
        DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions)
install(FILES packaging/mcenter-doctor.service packaging/mcenter-battery.service DESTINATION lib/systemd/system)
install(FILES packaging/70-mcenter-rgb.rules packaging/71-mcenter-ddc.rules DESTINATION lib/udev/rules.d)
install(FILES packaging/mcenter-i2c.conf DESTINATION lib/modules-load.d)
install(FILES packaging/mcenter-autostart.desktop DESTINATION /etc/xdg/autostart RENAME mcenter.desktop)
install(DIRECTORY packaging/msi-wmi-platform/ DESTINATION /usr/src/msi-wmi-platform-1.0)      # DKMS source, built only where the kernel has no msi-wmi-platform
install(FILES data/bios-versions.json DESTINATION ${CMAKE_INSTALL_DATADIR}/mcenter)      # bundled fallback for the community BIOS-version hint; the app also tries to refresh this from GitHub

set(CPACK_PACKAGE_NAME "mcenter")
set(CPACK_PACKAGE_VENDOR "p34t1k")
set(CPACK_PACKAGE_CONTACT "Mcenter maintainers <https://github.com/p34t1k/Mcenter>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Control center for MSI laptops: modes, fans, battery, keyboard lighting, displays")
set(CPACK_PACKAGE_DESCRIPTION "Mcenter controls MSI laptops through the msi-ec kernel driver: power modes, fan curves, charge limit and automatic battery calibration, Mystic Light keyboard lighting, refresh-rate and external-monitor control. Fork of MControlSuite/MControlCenter.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/p34t1k/Mcenter")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_GENERATOR "DEB;RPM")
# .deb
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "pkexec | policykit-1")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "dkms, build-essential")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/packaging/scripts/postinst;${CMAKE_SOURCE_DIR}/packaging/scripts/prerm;${CMAKE_SOURCE_DIR}/packaging/scripts/postrm")
# .rpm
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
set(CPACK_RPM_PACKAGE_REQUIRES "polkit")
set(CPACK_RPM_PACKAGE_SUGGESTS "dkms, gcc, make")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/packaging/scripts/postinst")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/packaging/scripts/prerm")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/packaging/scripts/postrm")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/share/applications;/usr/libexec;/usr/src;/usr/lib/systemd;/usr/lib/systemd/system;/usr/lib/udev;/usr/lib/udev/rules.d;/usr/lib/modules-load.d;/usr/share/polkit-1;/usr/share/polkit-1/actions;/usr/share/dbus-1;/usr/share/dbus-1/system.d;/usr/share/dbus-1/system-services;/usr/share/metainfo;/usr/share/icons;/usr/share/icons/hicolor;/usr/share/icons/hicolor/scalable;/usr/share/icons/hicolor/scalable/apps;/etc/xdg;/etc/xdg/autostart")
include(CPack)

# Optional: keyboard/accessibility unit test (not built by default). cmake --build build --target a11y_test && QT_QPA_PLATFORM=offscreen build/a11y_test
find_package(Qt6 QUIET COMPONENTS Test)
if(Qt6Test_FOUND)
    qt_add_executable(a11y_test EXCLUDE_FROM_ALL tests/a11y-test.cpp src/a11y.cpp src/a11y.h src/uikit.h)
    target_link_libraries(a11y_test PRIVATE Qt6::Widgets Qt6::Test)
    set_target_properties(a11y_test PROPERTIES AUTOMOC ON)
endif()
