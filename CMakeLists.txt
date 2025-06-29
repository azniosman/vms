cmake_minimum_required(VERSION 3.16)
project(vms VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Enable GUI components
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_WIDGETS_LIB")

# Security compiler flags
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong -D_FORTIFY_SOURCE=2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Wformat-security")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro,-z,now")
endif()

# Find required Qt packages
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Widgets
    Sql
    Network
    PrintSupport
)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Set source files
set(SOURCES
    src/main.cpp
    src/core/Visitor.cpp
    src/core/VisitorManager.cpp
    src/database/DatabaseManager.cpp
    src/security/SecurityManager.cpp
    src/utils/ErrorHandler.cpp
    src/reports/ReportManager.cpp
    src/gui/MainWindow.cpp
    src/gui/VisitorRegistrationDialog.cpp
    src/gui/CheckInOutDialog.cpp
)

# Set header files
set(HEADERS
    src/core/Visitor.h
    src/core/VisitorManager.h
    src/database/DatabaseManager.h
    src/security/SecurityManager.h
    src/utils/ErrorHandler.h
    src/reports/ReportManager.h
    src/gui/MainWindow.h
    src/gui/VisitorRegistrationDialog.h
    src/gui/CheckInOutDialog.h
)

# Set resource files
set(RESOURCES
    resources/icons.qrc
    resources/styles.qrc
)

# Create executable
add_executable(${PROJECT_NAME} 
    ${SOURCES}
    ${HEADERS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::PrintSupport
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
) 