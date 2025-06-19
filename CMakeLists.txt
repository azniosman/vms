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
    Multimedia
    MultimediaWidgets
)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Set source files
set(SOURCES
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/VisitorRegistrationDialog.cpp
    src/gui/CheckInOutDialog.cpp
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
    src/gui/MainWindow.h
    src/gui/VisitorRegistrationDialog.h
    src/gui/CheckInOutDialog.h
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
    Qt6::Multimedia
    Qt6::MultimediaWidgets
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

# Add resources
set(RESOURCE_FILES
    resources/icons/app.png
    resources/icons/visitor.png
    resources/icons/checkin.png
    resources/icons/checkout.png
    resources/icons/print.png
    resources/icons/settings.png
    resources/icons/report.png
    resources/icons/logout.png
)

qt_add_resources(${PROJECT_NAME} "resources"
    PREFIX "/"
    FILES ${RESOURCE_FILES}
) 