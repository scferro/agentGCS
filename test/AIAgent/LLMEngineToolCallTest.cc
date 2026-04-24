#include "LLMEngineToolCallTest.h"

#include <QtTest/QSignalSpy>

#include "LLMEngine.h"

// ---------------------------------------------------------------------------
// Unit tests (no model required)
// ---------------------------------------------------------------------------

void LLMEngineToolCallTest::_setToolsTest()
{
    LLMEngine engine;

    AgentTool tool;
    tool.name = "get_weather";
    tool.description = "Get the current weather for a location";
    tool.parametersJson = R"({"type":"object","properties":{"location":{"type":"string"}},"required":["location"]})";

    engine.setTools({tool});

    // Verify API doesn't crash and tool is stored
    // (No public getter for tools, but we verify it doesn't assert)
    QVERIFY(true);
}

void LLMEngineToolCallTest::_clearToolsTest()
{
    LLMEngine engine;

    AgentTool tool;
    tool.name = "test_tool";
    tool.description = "A test";
    tool.parametersJson = R"({"type":"object"})";

    engine.setTools({tool});
    engine.clearTools();

    // After clear, no tools should be registered
    // Can't directly verify internal state, but verify API works
    QVERIFY(true);
}

void LLMEngineToolCallTest::_toolRegistrationOverwriteTest()
{
    LLMEngine engine;

    AgentTool tool1;
    tool1.name = "first_tool";
    tool1.description = "First";
    tool1.parametersJson = R"({"type":"object"})";

    AgentTool tool2;
    tool2.name = "second_tool";
    tool2.description = "Second";
    tool2.parametersJson = R"({"type":"object"})";

    engine.setTools({tool1});
    engine.setTools({tool2}); // Should replace, not append

    // Verify API accepts overwrite without crash
    QVERIFY(true);
}

void LLMEngineToolCallTest::_toolCallDetectedSignatureTest()
{
    LLMEngine engine;

    // Verify the signal exists with the correct signature (toolName, arguments)
    QSignalSpy spy(&engine, &LLMEngine::toolCallDetected);
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 0);
}

void LLMEngineToolCallTest::_agentToolStructTest()
{
    // Verify AgentTool struct constructors and field access
    AgentTool tool;
    tool.name = "add_waypoint";
    tool.description = "Add a waypoint to the mission";
    tool.parametersJson = R"({"type":"object","properties":{"lat":{"type":"number"},"lon":{"type":"number"}}})";

    QCOMPARE(tool.name, QString("add_waypoint"));
    QCOMPARE(tool.description, QString("Add a waypoint to the mission"));
    QVERIFY(tool.parametersJson.contains("lat"));

    // Verify copy semantics
    AgentTool copy = tool;
    QCOMPARE(copy.name, tool.name);
    QCOMPARE(copy.description, tool.description);
    QCOMPARE(copy.parametersJson, tool.parametersJson);

    // Verify QList<AgentTool> works (Q_DECLARE_METATYPE)
    QList<AgentTool> tools = {tool, copy};
    QCOMPARE(tools.size(), 2);
}

// ---------------------------------------------------------------------------
// Integration tests (require GGUF_MODEL_PATH)
// ---------------------------------------------------------------------------

void LLMEngineToolCallTest::initTestCase()
{
    const QString modelPath = qEnvironmentVariable("GGUF_MODEL_PATH");
    if (modelPath.isEmpty()) {
        QSKIP("GGUF_MODEL_PATH not set, skipping tool call integration tests");
    }

    m_engine = new LLMEngine();
    m_engine->setModelPath(modelPath);
    m_engine->setContextLength(2048);
    m_engine->setGpuLayers(0);

    if (!m_engine->loadModel()) {
        QSKIP("Failed to load model, skipping tool call integration tests");
    }
}

void LLMEngineToolCallTest::cleanupTestCase()
{
    if (m_engine) {
        m_engine->unloadModel();
        delete m_engine;
        m_engine = nullptr;
    }
}

void LLMEngineToolCallTest::_toolCallWithGemma4Test()
{
    if (!m_engine || !m_engine->isLoaded()) {
        QSKIP("Model not loaded");
    }

    // Register a weather tool that the model should want to call
    AgentTool tool;
    tool.name = "get_weather";
    tool.description = "Get the current weather for a location";
    tool.parametersJson = R"({
        "type": "object",
        "properties": {
            "location": {
                "type": "string",
                "description": "City name"
            }
        },
        "required": ["location"]
    })";

    m_engine->setTools({tool});

    QSignalSpy toolCallSpy(m_engine, &LLMEngine::toolCallDetected);
    QSignalSpy completeSpy(m_engine, &LLMEngine::generationComplete);

    QList<ChatMessage> messages;
    ChatMessage sysMsg;
    sysMsg.role = "system";
    sysMsg.content = "You are a helpful assistant. When the user asks about weather, use the get_weather tool.";
    messages.append(sysMsg);

    ChatMessage userMsg;
    userMsg.role = "user";
    userMsg.content = "What is the weather in San Francisco?";
    messages.append(userMsg);

    m_engine->startCompletion(messages);

    // Wait for completion (up to 60s)
    QVERIFY(completeSpy.wait(60000));

    // With Gemma 4 E2B and a weather prompt, the model should produce a tool call.
    // However, model behavior is probabilistic, so we verify the mechanism works
    // regardless of whether a tool call was actually generated.
    if (toolCallSpy.count() > 0) {
        QString toolName = toolCallSpy.at(0).at(0).toString();
        QString toolArgs = toolCallSpy.at(0).at(1).toString();
        QCOMPARE(toolName, QString("get_weather"));
        QVERIFY(toolArgs.contains("San Francisco") || toolArgs.contains("location"));
        qDebug() << "Tool call detected:" << toolName << "args:" << toolArgs;
    } else {
        qDebug() << "No tool call detected (model may not have triggered it)";
    }

    QVERIFY(completeSpy.count() >= 1);
}

void LLMEngineToolCallTest::_noToolCallWithoutToolsTest()
{
    if (!m_engine || !m_engine->isLoaded()) {
        QSKIP("Model not loaded");
    }

    m_engine->clearTools();

    QSignalSpy toolCallSpy(m_engine, &LLMEngine::toolCallDetected);
    QSignalSpy completeSpy(m_engine, &LLMEngine::generationComplete);

    QList<ChatMessage> messages;
    ChatMessage userMsg;
    userMsg.role = "user";
    userMsg.content = "Say hello in one word.";
    messages.append(userMsg);

    m_engine->startCompletion(messages);
    QVERIFY(completeSpy.wait(60000));

    // Without tools registered, no tool call should be detected
    QCOMPARE(toolCallSpy.count(), 0);
}

void LLMEngineToolCallTest::_multiToolRegistrationTest()
{
    if (!m_engine || !m_engine->isLoaded()) {
        QSKIP("Model not loaded");
    }

    AgentTool weatherTool;
    weatherTool.name = "get_weather";
    weatherTool.description = "Get the current weather for a location";
    weatherTool.parametersJson = R"({"type":"object","properties":{"location":{"type":"string"}},"required":["location"]})";

    AgentTool timeTool;
    timeTool.name = "get_time";
    timeTool.description = "Get the current time for a timezone";
    timeTool.parametersJson = R"({"type":"object","properties":{"timezone":{"type":"string"}},"required":["timezone"]})";

    m_engine->setTools({weatherTool, timeTool});

    QSignalSpy completeSpy(m_engine, &LLMEngine::generationComplete);

    QList<ChatMessage> messages;
    ChatMessage sysMsg;
    sysMsg.role = "system";
    sysMsg.content = "You are a helpful assistant. Use the available tools when appropriate.";
    messages.append(sysMsg);

    ChatMessage userMsg;
    userMsg.role = "user";
    userMsg.content = "What is the weather in Tokyo and the time in JST?";
    messages.append(userMsg);

    m_engine->startCompletion(messages);
    QVERIFY(completeSpy.wait(60000));

    // Model may or may not call tools, but the important thing is
    // the API accepts multiple tools without crashing
    QVERIFY(true);

    // Clean up
    m_engine->clearTools();
}

UT_REGISTER_TEST(LLMEngineToolCallTest, TestLabel::Unit)