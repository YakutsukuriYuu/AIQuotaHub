cmake_minimum_required(VERSION 3.21)

project(AIQuotaHub VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Test)

qt_standard_project_setup()

qt_add_executable(AIQuotaHub MACOSX_BUNDLE
    src/main.cpp
    src/core/Models.h
    src/core/JsonPath.h
    src/core/Provider.h
    src/core/HttpJsonClient.h
    src/core/HttpJsonClient.cpp
    src/core/ProvidersConfig.h
    src/core/ProvidersConfig.cpp
    src/core/CredentialStore.h
    src/core/CredentialStore.cpp
    src/providers/GlmParser.h
    src/providers/GlmParser.cpp
    src/providers/GenericBalanceParser.h
    src/providers/GenericBalanceParser.cpp
    src/providers/OpenAiCostsParser.h
    src/providers/OpenAiCostsParser.cpp
    src/providers/CustomJsonParser.h
    src/providers/CustomJsonParser.cpp
    src/providers/HttpProvider.h
    src/providers/HttpProvider.cpp
    src/providers/DemoProvider.h
    src/providers/DemoProvider.cpp
    src/core/ProviderManager.h
    src/core/ProviderManager.cpp
    src/core/RefreshScheduler.h
    src/core/RefreshScheduler.cpp
    src/ui/UiColors.h
    src/ui/Theme.h
    src/ui/Theme.cpp
    src/ui/RingProgress.h
    src/ui/RingProgress.cpp
    src/ui/ProviderCard.h
    src/ui/ProviderCard.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp
    src/ui/AddProviderDialog.h
    src/ui/AddProviderDialog.cpp
    src/ui/TrayIcon.h
    src/ui/TrayIcon.cpp
)

set_target_properties(AIQuotaHub PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "app.aiquotahub"
    MACOSX_BUNDLE_BUNDLE_NAME "AI Quota Hub"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

# providers.json 打进 App 包 Resources（用户可在 Application Support 放同名文件覆盖）
set(PROVIDERS_JSON ${CMAKE_CURRENT_SOURCE_DIR}/resources/providers.json)
set_source_files_properties(${PROVIDERS_JSON} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
target_sources(AIQuotaHub PRIVATE ${PROVIDERS_JSON})

target_link_libraries(AIQuotaHub PRIVATE
    Qt6::Widgets
    Qt6::Network
)

enable_testing()
add_subdirectory(tests)
