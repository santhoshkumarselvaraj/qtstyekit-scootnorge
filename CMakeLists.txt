cmake_minimum_required(VERSION 3.21)

project(ScootNorge VERSION 0.1 LANGUAGES CXX)

# StyleKit (Qt.labs.StyleKit) ships with Qt 6.11+
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Quick Network Positioning Location)

qt_standard_project_setup(REQUIRES 6.11)

# Entur asks every client to identify itself ("company-application").
set(ET_CLIENT_NAME "personal-scootnorge" CACHE STRING "Value sent in the ET-Client-Name header")

qt_add_executable(appScootNorge
    src/main.cpp
)

set_source_files_properties(qml/Fmt.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(appScootNorge
    URI ScootNorge
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Fmt.qml
        qml/ScootStyle.qml
        qml/OperatorVariation.qml
        qml/OperatorChipVariation.qml
        qml/MapPage.qml
        qml/NearbyPage.qml
        qml/ComparePage.qml
        qml/FilterSheet.qml
        qml/VehicleCard.qml
        qml/ZoneLegend.qml
    SOURCES
        src/operators.h
        src/gbfs.h src/gbfs.cpp
        src/vehiclemodel.h src/vehiclemodel.cpp
        src/zonemodel.h src/zonemodel.cpp
        src/mobilityservice.h src/mobilityservice.cpp
)

# The generated QML type registration code does #include <mobilityservice.h> etc.,
# so the directory containing the QML_ELEMENT headers must be on the include path.
target_include_directories(appScootNorge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(appScootNorge PRIVATE ET_CLIENT_NAME="${ET_CLIENT_NAME}")

target_link_libraries(appScootNorge PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Network
    Qt6::Positioning
    Qt6::Location
)

set_target_properties(appScootNorge PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    QT_ANDROID_PACKAGE_NAME "no.scootnorge.app"
    QT_ANDROID_APP_NAME "ScootNorge"
)

# HTTPS on Android needs OpenSSL bundled with the APK.
if(ANDROID)
    include(FetchContent)
    FetchContent_Declare(android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(appScootNorge)
endif()

include(GNUInstallDirs)
install(TARGETS appScootNorge
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
