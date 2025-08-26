include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(typestr)


add_executable(vibeinput
        vibeinput.cc
        ${CMAKE_CURRENT_LIST_DIR}/../../sherpa-onnx/csrc/microphone.cc
)
target_link_libraries(vibeinput
        sherpa-onnx-cxx-api
        portaudio_static
        typestr
)



