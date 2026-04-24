#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

struct llama_model;
struct llama_context;
struct llama_sampler;
struct common_chat_templates;

/// @brief Qt wrapper around llama.cpp model loading, context, and chat templates.
///
/// LLMEngine manages the lifecycle of a llama.cpp model and its inference context.
/// It exposes load/unload operations as Q_INVOKABLE methods and emits Qt signals
/// for state changes and errors.
///
/// The engine is designed to be moved to a background QThread for non-blocking
/// inference (completion loop added in PR 4).
class LLMEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoaded READ isLoaded NOTIFY isLoadedChanged)
    Q_PROPERTY(bool isGenerating READ isGenerating NOTIFY isGeneratingChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(int contextLength READ contextLength WRITE setContextLength NOTIFY contextLengthChanged)
    Q_PROPERTY(int gpuLayers READ gpuLayers WRITE setGpuLayers NOTIFY gpuLayersChanged)

public:
    explicit LLMEngine(QObject* parent = nullptr);
    ~LLMEngine();

    bool isLoaded() const { return m_isLoaded; }
    bool isGenerating() const { return m_isGenerating; }
    QString modelPath() const { return m_modelPath; }
    int contextLength() const { return m_contextLength; }
    int gpuLayers() const { return m_gpuLayers; }

    void setModelPath(const QString& path);
    void setContextLength(int n);
    void setGpuLayers(int n);

    /// Load the GGUF model from modelPath(). Creates the llama context and
    /// initializes chat templates for Gemma 4 tool-calling support.
    /// Returns true on success, false on failure (emits loadFailed).
    Q_INVOKABLE bool loadModel();

    /// Unload the current model and free all llama.cpp resources.
    Q_INVOKABLE void unloadModel();

signals:
    void isLoadedChanged();
    void isGeneratingChanged();
    void modelPathChanged();
    void contextLengthChanged();
    void gpuLayersChanged();
    void loadFailed(const QString& error);

    // Added in PR 4 (streaming completion):
    // void tokenGenerated(const QString& token);
    // void generationComplete(const QString& fullText);
    // void toolCallDetected(const QString& toolName, const QString& arguments);

private:
    bool m_isLoaded = false;
    bool m_isGenerating = false;
    QString m_modelPath;
    int m_contextLength = 4096;
    int m_gpuLayers = 0;

    // Owned llama.cpp objects (raw pointers — freed in destructor/unloadModel)
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    common_chat_templates* m_chatTemplates = nullptr;
};