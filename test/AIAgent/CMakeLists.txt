# ============================================================================
# AIAgent Unit Tests
# ============================================================================
# Tests for LLMEngine model loading, streaming completion, and tool calling.
#
# LLMEngine load tests run without a model file (they test error paths).
# Streaming/completion tests require a GGUF model file set via
# GGUF_MODEL_PATH environment variable, and will be skipped if not set.
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LLMEngineLoadTest.cc
        LLMEngineLoadTest.h
        LLMEngineStreamingTest.cc
        LLMEngineStreamingTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/AIAgent
)