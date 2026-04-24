#pragma once

#include "UnitTest.h"

/// @brief Tests for LLMEngine model loading and unloading.
///
/// These tests validate basic LLMEngine functionality:
/// - Construction with default property values
/// - Error handling for missing/invalid model paths
/// - Property change signal emission
/// - Safe unload without prior load
///
/// Tests requiring a real GGUF model file need the GGUF_MODEL_PATH
/// environment variable set. They will be skipped otherwise.
class LLMEngineLoadTest : public UnitTest
{
    Q_OBJECT

public:
    LLMEngineLoadTest() : UnitTest("LLMEngineLoadTest", {TestLabel::Unit}) {}

private slots:
    void _constructionTest();
    void _loadNonexistentModelTest();
    void _loadEmptyPathTest();
    void _propertyChangeTest();
    void _unloadWithoutLoadTest();
    void _loadRealModelTest();
};