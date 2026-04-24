#pragma once

#include "UnitTest.h"
#include "LLMEngine.h"

/// @brief Tests for LLMEngine streaming completion and token generation.
///
/// All tests require a real GGUF model (set GGUF_MODEL_PATH env var).
/// Tests will be skipped if the env var is not set.
class LLMEngineStreamingTest : public UnitTest
{
    Q_OBJECT

public:
    LLMEngineStreamingTest() = default;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void _tokenSignalsTest();
    void _completionTextMatchesTokensTest();
    void _cancellationTest();
    void _generatingStateTest();
    void _systemMessageCompletionTest();

private:
    LLMEngine* m_engine = nullptr;
};