cmake_minimum_required(VERSION 3.24)

project(fibonacci_algorithm_with_cuda
    VERSION 0.1.0
    DESCRIPTION "CPU and GPU Fibonacci benchmark with a Qt 6 GUI"
    LANGUAGES CXX CUDA
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

include(CTest)

add_library(fibonacci_io STATIC src/benchmark_io.cpp)
target_include_directories(fibonacci_io PUBLIC src)

add_library(fibonacci_core STATIC src/fibonacci_core.cu)
target_include_directories(fibonacci_core PUBLIC src)
set_target_properties(fibonacci_core PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

add_executable(fibonacci_cuda src/main.cpp)
target_link_libraries(fibonacci_cuda PRIVATE fibonacci_core fibonacci_io)

option(BUILD_GUI "Build the Qt 6 desktop application" ON)

if(BUILD_GUI)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Charts)
    qt_standard_project_setup()

    qt_add_library(fibonacci_gui_lib OBJECT
        src/gui/benchmark_worker.cpp
        src/gui/benchmark_worker.h
        src/gui/chart_widget.cpp
        src/gui/chart_widget.h
        src/gui/main_window.cpp
        src/gui/main_window.h
    )
    target_include_directories(fibonacci_gui_lib PUBLIC src src/gui)
    target_link_libraries(fibonacci_gui_lib PUBLIC
        fibonacci_core
        fibonacci_io
        Qt6::Widgets
        Qt6::Charts
    )

    qt_add_executable(fibonacci_gui src/gui/main.cpp)
    target_link_libraries(fibonacci_gui PRIVATE fibonacci_gui_lib)
    target_compile_definitions(fibonacci_gui PRIVATE
        FIB_VERSION="${PROJECT_VERSION}"
    )
    set_target_properties(fibonacci_gui PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
    )
endif()

include(GNUInstallDirs)

install(TARGETS fibonacci_cuda
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_GUI)
    install(TARGETS fibonacci_gui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(FILES packaging/fibonacci-cuda-benchmark.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES packaging/fibonacci-cuda-benchmark.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    )
endif()

set(CPACK_PACKAGE_NAME "fibonacci-cuda-benchmark")
set(CPACK_PACKAGE_VENDOR "Amir Khoshdel Louyeh")
set(CPACK_PACKAGE_CONTACT "Amir Khoshdel Louyeh")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "CPU and GPU Fibonacci benchmark with a Qt 6 GUI"
)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
include(CPack)

if(BUILD_TESTING)
    add_executable(fibonacci_core_test tests/test_fibonacci_core.cpp)
    target_link_libraries(fibonacci_core_test PRIVATE fibonacci_core)

    add_test(
        NAME fibonacci_core_correctness
        COMMAND fibonacci_core_test
    )

    add_executable(fibonacci_io_test tests/test_benchmark_io.cpp)
    target_link_libraries(fibonacci_io_test PRIVATE fibonacci_io)

    add_test(
        NAME fibonacci_io_round_trip
        COMMAND fibonacci_io_test
    )

    add_test(
        NAME fibonacci_help
        COMMAND fibonacci_cuda --help
    )
    set_tests_properties(fibonacci_help PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage:"
    )

    add_test(
        NAME fibonacci_rejects_invalid_count
        COMMAND fibonacci_cuda --count 0
    )
    set_tests_properties(fibonacci_rejects_invalid_count PROPERTIES
        PASS_REGULAR_EXPRESSION "--count must be between"
    )

    if(BUILD_GUI)
        find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

        qt_add_executable(fibonacci_gui_test tests/test_gui_smoke.cpp)
        target_link_libraries(fibonacci_gui_test PRIVATE
            fibonacci_gui_lib
            Qt6::Test
        )

        add_test(
            NAME fibonacci_gui_smoke
            COMMAND fibonacci_gui_test
        )
        set_tests_properties(fibonacci_gui_smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )
    endif()
endif()
