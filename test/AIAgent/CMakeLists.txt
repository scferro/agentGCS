# ============================================================================
# AIAgent Unit Tests
# ============================================================================
# Tests for LLMEngine model loading, streaming completion, and tool calling.
#
# LLMEngine load tests run without a model file (they test error paths).
# Streaming/completion tests require a GGUF model file set via
# GGUF_MODEL_PATH environment variable, and will be skipped if not set.
# ============================================================================

# QGC unit tests (linked into QGroundControl binary, run via --unittest:*)
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LLMEngineLoadTest.cc
        LLMEngineLoadTest.h
        LLMEngineStreamingTest.cc
        LLMEngineStreamingTest.h
        LLMEngineToolCallTest.cc
        LLMEngineToolCallTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/AIAgent
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/include
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/common
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/ggml/include
)

# Suppress -Wshadow and -Wunused-function warnings from llama.cpp headers.
# QGC builds with -Werror which flags these as errors in llama.cpp's chat.h
# and jinja headers (variable shadowing, unused static functions).
set_source_files_properties(
    LLMEngineLoadTest.cc
    LLMEngineStreamingTest.cc
    LLMEngineToolCallTest.cc
    PROPERTIES COMPILE_FLAGS "-Wno-shadow -Wno-unused-function"
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        nlohmann_json::nlohmann_json
)

# ============================================================================
# Standalone smoke tests (separate executables, not part of QGC binary)
# ============================================================================
# These are minimal executables that test LLMEngine directly with a model
# file, without requiring the full QGC test harness. Run them in Docker:
#   ./test_model_load /models/SmolLM2-135M-Instruct-Q4_K_M.gguf
#   ./test_tool_call /models/gemma-4-E2B-it-Q4_K_M.gguf --test tools
#
# NOTE: These targets only exist when QGC_BUILD_TESTING=ON, which requires
# CMAKE_BUILD_TYPE=Debug (see cmake/CustomOptions.cmake: cmake_dependent_option).
# ============================================================================

# test_model_load: Minimal llama.cpp smoke test (no Qt, no LLMEngine)
add_executable(test_model_load
    test_model_load.cpp
)
target_link_libraries(test_model_load
    PRIVATE
        llama
        llama-common
        ggml
        ggml-base
        ggml-cpu
        stdc++fs
)
target_include_directories(test_model_load
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/include
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/common
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/ggml/include
)

# test_tool_call: LLMEngine tool calling smoke test (requires Qt)
add_executable(test_tool_call
    test_tool_call.cpp
)
target_link_libraries(test_tool_call
    PRIVATE
        AIAgentModule
        Qt6::Core
        Qt6::Core5Compat
        llama
        llama-common
        ggml
        ggml-base
        ggml-cpu
        nlohmann_json::nlohmann_json
        stdc++fs
)
target_include_directories(test_tool_call
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/AIAgent
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/include
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/common
        ${CMAKE_SOURCE_DIR}/extern/llama.cpp/ggml/include
)

# test_agent_tool_registry: Tool registry filtering tests (no model, no LLM)
add_executable(test_agent_tool_registry
    test_agent_tool_registry.cpp
)
target_link_libraries(test_agent_tool_registry
    PRIVATE
        AIAgentModule
        Qt6::Core
        nlohmann_json::nlohmann_json
)
target_include_directories(test_agent_tool_registry
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/AIAgent
)

# test_add_tools_schema: Standalone schema/availability test for add tools.
# Uses stub tool classes (no execute() deps needed) so it can run headlessly
# in Docker without the full QGC codebase.
add_executable(test_add_tools_schema
    test_add_tools_schema.cpp
)
target_link_libraries(test_add_tools_schema
    PRIVATE
        AIAgentModule
        Qt6::Core
        nlohmann_json::nlohmann_json
)
target_include_directories(test_add_tools_schema
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/AIAgent
)

# QGC-internal mission add tools test (compiles into QGroundControl binary)
# Run via: ./QGroundControl --unittest:AddToolsTest
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AddToolsTest.cc
        AddToolsTest.h
)
target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/AIAgent/Tools
)