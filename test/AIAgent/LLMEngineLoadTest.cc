#include "LLMEngineLoadTest.h"

#include <QtTest/QSignalSpy>

#include "LLMEngine.h"

void LLMEngineLoadTest::_constructionTest()
{
    LLMEngine engine;
    QVERIFY(!engine.isLoaded());
    QVERIFY(!engine.isGenerating());
    QCOMPARE(engine.modelPath(), QString());
    QCOMPARE(engine.contextLength(), 4096);
    QCOMPARE(engine.gpuLayers(), 0);
}

void LLMEngineLoadTest::_loadNonexistentModelTest()
{
    LLMEngine engine;
    engine.setModelPath("/nonexistent/path/model.gguf");

    QSignalSpy spy(&engine, &LLMEngine::loadFailed);
    bool result = engine.loadModel();

    QVERIFY(!result);
    QVERIFY(!engine.isLoaded());
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.at(0).at(0).toString().contains("not found"));
}

void LLMEngineLoadTest::_loadEmptyPathTest()
{
    LLMEngine engine;

    QSignalSpy spy(&engine, &LLMEngine::loadFailed);
    bool result = engine.loadModel();

    QVERIFY(!result);
    QVERIFY(!engine.isLoaded());
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.at(0).at(0).toString().contains("No model path"));
}

void LLMEngineLoadTest::_propertyChangeTest()
{
    LLMEngine engine;

    QSignalSpy pathSpy(&engine, &LLMEngine::modelPathChanged);
    QSignalSpy ctxSpy(&engine, &LLMEngine::contextLengthChanged);
    QSignalSpy gpuSpy(&engine, &LLMEngine::gpuLayersChanged);

    engine.setModelPath("/some/model.gguf");
    engine.setContextLength(8192);
    engine.setGpuLayers(99);

    QCOMPARE(engine.modelPath(), QString("/some/model.gguf"));
    QCOMPARE(engine.contextLength(), 8192);
    QCOMPARE(engine.gpuLayers(), 99);

    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(ctxSpy.count(), 1);
    QCOMPARE(gpuSpy.count(), 1);

    // Setting same values should not re-emit
    engine.setModelPath("/some/model.gguf");
    engine.setContextLength(8192);
    engine.setGpuLayers(99);

    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(ctxSpy.count(), 1);
    QCOMPARE(gpuSpy.count(), 1);
}

void LLMEngineLoadTest::_unloadWithoutLoadTest()
{
    LLMEngine engine;
    engine.unloadModel();
    QVERIFY(!engine.isLoaded());
}

void LLMEngineLoadTest::_loadRealModelTest()
{
    const QString modelPath = qEnvironmentVariable("GGUF_MODEL_PATH");
    if (modelPath.isEmpty()) {
        QSKIP("GGUF_MODEL_PATH not set, skipping real model load test");
    }

    LLMEngine engine;
    engine.setModelPath(modelPath);
    engine.setContextLength(2048);
    engine.setGpuLayers(0);

    QVERIFY(engine.loadModel());
    QVERIFY(engine.isLoaded());

    engine.unloadModel();
    QVERIFY(!engine.isLoaded());
}

UT_REGISTER_TEST(LLMEngineLoadTest, TestLabel::Unit)