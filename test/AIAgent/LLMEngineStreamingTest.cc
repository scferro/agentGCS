#include "LLMEngineStreamingTest.h"

#include <QtTest/QSignalSpy>

#include "LLMEngine.h"

void LLMEngineStreamingTest::initTestCase()
{
    const QString modelPath = qEnvironmentVariable("GGUF_MODEL_PATH");
    if (modelPath.isEmpty()) {
        QSKIP("GGUF_MODEL_PATH not set, skipping all streaming tests");
    }

    m_engine = new LLMEngine(this);
    m_engine->setModelPath(modelPath);
    m_engine->setContextLength(2048);
    m_engine->setGpuLayers(0);

    QVERIFY2(m_engine->loadModel(), "Failed to load model");
}

void LLMEngineStreamingTest::cleanupTestCase()
{
    if (m_engine) {
        m_engine->unloadModel();
        delete m_engine;
        m_engine = nullptr;
    }
}

void LLMEngineStreamingTest::_tokenSignalsTest()
{
    if (!m_engine || !m_engine->isLoaded()) {
        QSKIP("Model not loaded");
    }

    QList<ChatMessage> messages;
    messages.append({"user", "Say hello in one word.", "", ""});

    QSignalSpy tokenSpy(m_engine, &LLMEngine::tokenGenerated);
    QSignalSpy completeSpy(m_engine, &LLMEngine::generationComplete);

    m_engine->startCompletion(messages);

    QVERIFY(completeSpy.wait(30000));

    QVERIFY(tokenSpy.count() > 0);
    QCOMPARE(completeSpy.count(), 1);

    QString fullText = completeSpy.at(0).at(0).toString();
    QVERIFY(!fullText.isEmpty());
}

void LLMEngineStreamingTest::_completionTextMatchesTokensTest()
{
    if (!m_engine || !m_engine->isLoaded()) {
        QSKIP("Model not loaded");
    }

    QList<ChatMessage> messages;
    messages.append({"user", "What is 2+2? Answer with just the number.", "", ""});

    QSignalSpy tokenSpy(m_engine, &LLMEngine::tokenGenerated);
    QSignalSpy completeSpy(m_engine, &LLMEngine::generationComplete);

    m_engine->startCompletion(messages);
    QVERIFY(completeSpy.wait(30000));

    QString concatenated;
    for (int i = 0; i < tokenSpy.count(); ++i) {
        concatenated += tokenSpy.at(i).at(0).toString();
    }

    QString fullText = completeSpy.at(0).at(0).toString();
    QCOMPARE(concatenated, fullText);
}

void LLMEngineStreamingTest::_cancellationTest()
{
    if (!m_engine || !m_engine->isLoaded()) {
        QSKIP("Model not loaded");
    }

    QList<ChatMessage> messages;
    messages.append({"user", "Count from 1 to 100, one number per line.", "", ""});

    QSignalSpy tokenSpy(m_engine, &LLMEngine::tokenGenerated);
    QSignalSpy completeSpy(m_engine, &LLMEngine::generationComplete);

    m_engine->startCompletion(messages);

    // Wait for a few tokens then cancel
    QVERIFY(tokenSpy.wait(5000));

    m_engine->cancelCompletion();

    QVERIFY(completeSpy.wait(5000));
}

void LLMEngineStreamingTest::_generatingStateTest()
{
    if (!m_engine || !m_engine->isLoaded()) {
        QSKIP("Model not loaded");
    }

    QVERIFY(!m_engine->isGenerating());

    QList<ChatMessage> messages;
    messages.append({"user", "Hi", "", ""});

    QSignalSpy completeSpy(m_engine, &LLMEngine::generationComplete);

    m_engine->startCompletion(messages);

    QVERIFY(completeSpy.wait(30000));
    QVERIFY(!m_engine->isGenerating());
}

void LLMEngineStreamingTest::_systemMessageCompletionTest()
{
    if (!m_engine || !m_engine->isLoaded()) {
        QSKIP("Model not loaded");
    }

    QList<ChatMessage> messages;
    messages.append({"system", "You are a helpful assistant. Be concise.", "", ""});
    messages.append({"user", "What color is the sky?", "", ""});

    QSignalSpy completeSpy(m_engine, &LLMEngine::generationComplete);
    m_engine->startCompletion(messages);
    QVERIFY(completeSpy.wait(30000));

    QString fullText = completeSpy.at(0).at(0).toString();
    QVERIFY(!fullText.isEmpty());
}

UT_REGISTER_TEST(LLMEngineStreamingTest, TestLabel::Unit)