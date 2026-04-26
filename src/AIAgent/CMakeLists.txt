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
        AgentToolBase.h
        AgentToolBase.cpp
        AgentToolRegistry.h
        AgentToolRegistry.cpp
        AgentController.h
        AgentController.cpp
        AgentPrompts.h
        AgentContextSerializer.h
        AgentContextSerializer.cpp
)

target_include_directories(AIAgentModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Mission add tools
add_subdirectory(Tools)

# QGC subsystem headers needed by tool implementations.
# The tools include MissionManager headers which have deep transitive include
# chains across many QGC subsystems (Vehicle, FactSystem, QmlControls, MAVLink,
# Settings, etc.) and also need generated headers like qgc_version.h and
# mavlink_types.h.
# The full set of include directories is propagated from the main project target
# at the end of src/CMakeLists.txt (after all subdirectories are processed).
# Here we only add src/ root and build dir as a baseline.
target_include_directories(AIAgentModule
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}
)

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
        Qt6::Positioning
        llama
        llama-common
        nlohmann_json::nlohmann_json
)