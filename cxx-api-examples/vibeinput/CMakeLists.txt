cmake_minimum_required(VERSION 3.16)

project(vibeinput VERSION 0.1 LANGUAGES CXX)

# Option to enable RNNoise denoising feature
option(VIBEINPUT_ENABLE_RNNOISE "Enable RNNoise denoising (builds bundled rnnoise and enables code)" OFF)

if (VIBEINPUT_ENABLE_RNNOISE)
  # ----------------------------------------------------------------------------
  # RNNoise via ExternalProject (local source under rnnoise/)
  # ----------------------------------------------------------------------------
  include(ExternalProject)

  set(RNNOISE_INSTALL_DIR ${CMAKE_BINARY_DIR}/rnnoise)

  # Platform-aware RNNoise optimization flags
  set(RNNOISE_CONFIGURE_FLAGS --disable-shared --enable-static --disable-examples --disable-doc)
  set(RNNOISE_CFLAGS "-fPIC")
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
      list(APPEND RNNOISE_CONFIGURE_FLAGS --enable-x86-rtcd)
      option(RNNOISE_ENABLE_AVX2 "Enable AVX2 (-mavx2) for RNNoise build (x86 only)" OFF)
      if (RNNOISE_ENABLE_AVX2)
          set(RNNOISE_CFLAGS "-O3 -mavx2")
      endif ()
  endif ()
  string(JOIN " " RNNOISE_CONFIGURE_FLAGS_STR ${RNNOISE_CONFIGURE_FLAGS})

  ExternalProject_Add(rnnoise_project
          SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/rnnoise
          DOWNLOAD_COMMAND ""

          # Autotools configure (out-of-source) with platform-aware flags
          CONFIGURE_COMMAND sh -c "cd <SOURCE_DIR> && ./autogen.sh && cd <BINARY_DIR> && env CFLAGS='${RNNOISE_CFLAGS}' <SOURCE_DIR>/configure  --prefix=<INSTALL_DIR> ${RNNOISE_CONFIGURE_FLAGS_STR}"

          # Build / Install
          BUILD_COMMAND make
          INSTALL_COMMAND make install

          # Inform CMake/Ninja that this file will be produced by the ExternalProject
          INSTALL_BYPRODUCTS ${RNNOISE_INSTALL_DIR}/lib/librnnoise.a

          INSTALL_DIR ${RNNOISE_INSTALL_DIR}
  )

  # Create IMPORTED static library target for rnnoise
  file(MAKE_DIRECTORY ${RNNOISE_INSTALL_DIR}/include)
  file(MAKE_DIRECTORY ${RNNOISE_INSTALL_DIR}/lib)
  add_library(rnnoise STATIC IMPORTED)
  set_property(TARGET rnnoise PROPERTY IMPORTED_LOCATION ${RNNOISE_INSTALL_DIR}/lib/librnnoise.a)
  set_property(TARGET rnnoise PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${RNNOISE_INSTALL_DIR}/include)
  if (UNIX AND NOT APPLE)
      set_property(TARGET rnnoise PROPERTY INTERFACE_LINK_LIBRARIES m)
  endif ()

  # Ensure rnnoise is built before being linked
  add_dependencies(rnnoise rnnoise_project)
endif ()

add_subdirectory(typestr)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add debug definition for file saving functionality
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(ENABLE_DEBUG_FILE_SAVE)
endif ()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

# Propagate detected Qt major version to the QHotkey subproject
set(QT_DEFAULT_MAJOR_VERSION ${QT_VERSION_MAJOR} CACHE STRING "Qt version to use (5 or 6)" FORCE)

add_subdirectory(QHotkey)

set(TS_FILES vibeinput_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp
        vibeinput.cc
        ${CMAKE_CURRENT_LIST_DIR}/../../sherpa-onnx/csrc/microphone.cc
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        preferenceform.cpp
        preferenceform.h
        preference.ui
        preference_manager.cpp
        preference_manager.h
        ${TS_FILES}
)

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(vibeinput
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
    )
    # Define target properties for Android with Qt 6 as:
    #    set_property(TARGET vibeinput APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
    #                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
    # For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else ()
    if (ANDROID)
        add_library(vibeinput SHARED
                ${PROJECT_SOURCES}
        )
        # Define properties for Android with Qt 5 after find_package() calls as:
        #    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else ()
        add_executable(vibeinput
                ${PROJECT_SOURCES}
        )
    endif ()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif ()

if (VIBEINPUT_ENABLE_RNNOISE)
  add_dependencies(vibeinput rnnoise_project)
  target_link_libraries(vibeinput PRIVATE rnnoise)
  target_compile_definitions(vibeinput PRIVATE VIBEINPUT_ENABLE_RNNOISE)
endif ()

target_link_libraries(vibeinput PRIVATE
        sherpa-onnx-cxx-api
        portaudio_static
        typestr
        QHotkey::QHotkey
)

target_link_libraries(vibeinput PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if (${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.vibeinput)
endif ()
set_target_properties(vibeinput PROPERTIES
        ${BUNDLE_ID_OPTION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS vibeinput
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(vibeinput)
endif ()
