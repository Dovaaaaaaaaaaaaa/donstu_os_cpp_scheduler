add_executable(donstu_os_cpp_scheduler
    main.cpp
    simulator.cpp
    process.h
    scheduler.h
    simulator.h
    testsets.h
    fcfs.h
    sjf.h
    srtn.h
    rr.h
    priority.h
    mlfq.h
)
cmake_minimum_required(VERSION 3.16)

project(donstu_os_cpp_scheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(donstu_os_cpp_scheduler PRIVATE -Wall -Wextra)
endif()
include(GNUInstallDirs)
install(TARGETS donstu_os_cpp_scheduler
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
