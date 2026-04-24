qt_add_library(AIAgentModule STATIC)

qt_add_qml_module(AIAgentModule
    URI QGroundControl.AIAgent
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        AIAgentSidebar.qml
        AIAgentChatView.qml
        AIAgentActionCard.qml
        AIAgentSlideToAccept.qml
    NO_PLUGIN
)

target_include_directories(AIAgentModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(AIAgentModule
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        llama
        llama-common
        nlohmann_json::nlohmann_json
)