#pragma once

#include "UnitTest.h"
#include "LLMEngine.h"

/// @brief Tests for LLMEngine tool registration and calling.
///
/// Unit tests (no model required):
/// - _setToolsTest: Verify setTools/clearTools API
/// - _toolRegistrationOverwrite: Verify calling setTools replaces prior tools
/// - _toolCallDetectedSignal: Verify toolCallDetected signal exists with correct signature
///
/// Integration tests (require GGUF_MODEL_PATH, uses Gemma 4 E2B):
/// - _toolCallWithGemma4: Register a tool, send a prompt that triggers it, verify detection
/// - _noToolCallWithoutTools: Verify no tool call when tools are empty
/// - _multiToolRegistration: Register multiple tools, verify all are available
class LLMEngineToolCallTest : public UnitTest
{
    Q_OBJECT

public:
    LLMEngineToolCallTest() = default;

private slots:
    void _setToolsTest();
    void _clearToolsTest();
    void _toolRegistrationOverwriteTest();
    void _toolCallDetectedSignatureTest();
    void _agentToolStructTest();

    // Integration tests require GGUF_MODEL_PATH
    void initTestCase();
    void cleanupTestCase();
    void _toolCallWithGemma4Test();
    void _noToolCallWithoutToolsTest();
    void _multiToolRegistrationTest();

private:
    LLMEngine* m_engine = nullptr;
};