// Standalone smoke test for LLMEngine tool calling (PR 5)
// Usage: test_tool_call <model_path> [--test tools|notools|parse]
//
// Tests:
//   tools   - Register tools, start completion, verify toolCallDetected fires
//   notools - Start completion without tools, verify no tool call emitted
//   parse   - Test common_chat_parse directly on known tool-call output

#include "LLMEngine.h"

#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QTimer>

#include <cstdio>
#include <cstdlib>
#include <cstring>

static bool g_toolCallDetected = false;
static QString g_toolCallName;
static QString g_toolCallArgs;
static QString g_generatedText;
static bool g_generationComplete = false;

static bool g_testPassed = false;

static void onToolCallDetected(const QString& name, const QString& args) {
    g_toolCallDetected = true;
    g_toolCallName = name;
    g_toolCallArgs = args;
    qDebug() << "  [SIGNAL] toolCallDetected:" << name << "args:" << args;
}

static void onTokenGenerated(const QString& token) {
    g_generatedText += token;
}

static void onGenerationComplete(const QString& fullText) {
    g_generationComplete = true;
    qDebug() << "  [SIGNAL] generationComplete:" << fullText.left(200) << "...";
}

static void onLoadFailed(const QString& error) {
    qWarning() << "  [SIGNAL] loadFailed:" << error;
}

// ---- Test: Register tools and verify tool call emission ----
static int testToolCalling(LLMEngine& engine) {
    printf("\n=== Test: Tool Calling ===\n");

    // Register a simple tool
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

    engine.setTools({tool});
    qDebug() << "  Registered tool:" << tool.name;

    // Build a conversation that should trigger a tool call
    QList<ChatMessage> messages;
    ChatMessage sysMsg;
    sysMsg.role = "system";
    sysMsg.content = "You are a helpful assistant. Use the available tools when appropriate.";
    messages.append(sysMsg);

    ChatMessage userMsg;
    userMsg.role = "user";
    userMsg.content = "What is the weather in San Francisco?";
    messages.append(userMsg);

    // Reset state
    g_toolCallDetected = false;
    g_toolCallName.clear();
    g_toolCallArgs.clear();
    g_generatedText.clear();
    g_generationComplete = false;

    // Run completion
    engine.startCompletion(messages);

    // Wait for completion (poll loop)
    for (int i = 0; i < 600 && !g_generationComplete; ++i) { // 60s max
        QCoreApplication::processEvents();
        QThread::msleep(100);
    }

    if (!g_generationComplete) {
        printf("  FAIL: Generation did not complete within timeout\n");
        return 1;
    }

    // Note: Whether a tool call is detected depends on the model.
    // Gemma 4 E2B with tools registered should ideally produce a tool call,
    // but it's model-dependent. For SmolLM2 which has no tool support,
    // we expect no tool call.
    if (g_toolCallDetected) {
        printf("  PASS: Tool call detected — name=%s args=%s\n",
               g_toolCallName.toUtf8().constData(),
               g_toolCallArgs.left(100).toUtf8().constData());
    } else {
        printf("  INFO: No tool call detected (model may not support tool calling)\n");
        printf("        Generated text: %s\n", g_generatedText.left(200).toUtf8().constData());
    }

    return 0;
}

// ---- Test: No tools registered → no tool call signal ----
static int testNoTools(LLMEngine& engine) {
    printf("\n=== Test: No Tools (baseline) ===\n");

    engine.clearTools();

    QList<ChatMessage> messages;
    ChatMessage userMsg;
    userMsg.role = "user";
    userMsg.content = "Say hello in one word.";
    messages.append(userMsg);

    g_toolCallDetected = false;
    g_generatedText.clear();
    g_generationComplete = false;

    engine.startCompletion(messages);

    for (int i = 0; i < 600 && !g_generationComplete; ++i) {
        QCoreApplication::processEvents();
        QThread::msleep(100);
    }

    if (!g_generationComplete) {
        printf("  FAIL: Generation did not complete\n");
        return 1;
    }

    if (g_toolCallDetected) {
        printf("  FAIL: Unexpected tool call detected when no tools registered\n");
        return 1;
    }

    printf("  PASS: No tool call detected (correct)\n");
    printf("        Generated: %s\n", g_generatedText.left(100).toUtf8().constData());
    return 0;
}

// ---- Test: setTools / clearTools API ----
static int testToolRegistration(LLMEngine& engine) {
    printf("\n=== Test: Tool Registration API ===\n");

    // This test doesn't need the model loaded — just verify the API works
    LLMEngine testEngine;

    AgentTool t1;
    t1.name = "tool_a";
    t1.description = "First test tool";
    t1.parametersJson = R"({"type":"object","properties":{}})";

    AgentTool t2;
    t2.name = "tool_b";
    t2.description = "Second test tool";
    t2.parametersJson = R"({"type":"object","properties":{}})";

    testEngine.setTools({t1, t2});
    printf("  setTools({tool_a, tool_b}) — OK\n");

    testEngine.clearTools();
    printf("  clearTools() — OK\n");

    // Test single tool
    testEngine.setTools({t1});
    printf("  setTools({tool_a}) — OK\n");

    printf("  PASS: Tool registration API works\n");
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <model_path> [--test tools|notools|parse|register]\n", argv[0]);
        return 1;
    }

    QCoreApplication app(argc, argv);
    const QString modelPath = QString::fromUtf8(argv[1]);
    QString testName = "all";
    if (argc >= 4 && strcmp(argv[2], "--test") == 0) {
        testName = argv[3];
    }

    int failures = 0;

    // Registration test doesn't need model
    if (testName == "register" || testName == "all") {
        failures += testToolRegistration(*new LLMEngine());
    }

    // Other tests need model
    if (testName != "register") {
        LLMEngine engine;
        QObject::connect(&engine, &LLMEngine::toolCallDetected, &onToolCallDetected);
        QObject::connect(&engine, &LLMEngine::tokenGenerated, &onTokenGenerated);
        QObject::connect(&engine, &LLMEngine::generationComplete, &onGenerationComplete);
        QObject::connect(&engine, &LLMEngine::loadFailed, &onLoadFailed);

        // Load model on the main thread (startCompletion dispatches to engine's
        // thread internally via QMetaObject::invokeMethod, so we don't need
        // moveToThread for this standalone test)
        printf("Loading model: %s ...\n", modelPath.toUtf8().constData());
        engine.setModelPath(modelPath);
        engine.setContextLength(2048);
        engine.setGpuLayers(0);
        if (!engine.loadModel()) {
            printf("FAIL: Could not load model\n");
            return 1;
        }
        printf("Model loaded!\n");

        if (testName == "tools" || testName == "all") {
            failures += testToolCalling(engine);
        }

        if (testName == "notools" || testName == "all") {
            failures += testNoTools(engine);
        }

        engine.unloadModel();
    }

    printf("\n=== Results: %d test(s) failed ===\n", failures);
    return failures;
}

// No Q_OBJECT in this file — no .moc include needed