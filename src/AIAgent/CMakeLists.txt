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

target_sources(AIAgentModule
    PRIVATE
        LLMEngine.h
        LLMEngine.cpp
        ModelManager.h
        ModelManager.cpp
        ModelDownloader.h
        ModelDownloader.cpp
)

target_include_directories(AIAgentModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# llama.cpp include directories (model loading, context, chat templates)
target_include_directories(AIAgentModule
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/include
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/common
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/ggml/include
)

target_link_libraries(AIAgentModule
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        Qt6::Network
        llama
        llama-common
        nlohmann_json::nlohmann_json
)