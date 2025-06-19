cmake_minimum_required(VERSION 3.16)
project(vms VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required Qt packages
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Gui
    Widgets
    Sql
    Network
    PrintSupport
)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Set source files (core only for now)
set(SOURCES
    src/main.cpp
    src/core/Visitor.cpp
    src/core/VisitorManager.cpp
    src/database/DatabaseManager.cpp
    src/security/SecurityManager.cpp
    src/utils/QRCodeGenerator.cpp
    src/utils/ErrorHandler.cpp
    src/reports/ReportManager.cpp
)

# Set header files
set(HEADERS
    src/core/Visitor.h
    src/core/VisitorManager.h
    src/database/DatabaseManager.h
    src/security/SecurityManager.h
    src/utils/QRCodeGenerator.h
    src/utils/ErrorHandler.h
    src/reports/ReportManager.h
)

# Create executable
add_executable(${PROJECT_NAME} 
    ${SOURCES}
    ${HEADERS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::PrintSupport
    OpenSSL::SSL
    OpenSSL::Crypto
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