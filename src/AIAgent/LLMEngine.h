#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <atomic>

struct llama_model;
struct llama_context;
struct llama_sampler;
struct common_chat_templates;
#include "chat.h" // for common_chat_params (needed by m_lastChatParams member)

/// @brief Data structure for chat messages passed to/from the LLM.
struct ChatMessage {
    QString role;       ///< "system", "user", "assistant", "tool"
    QString content;   ///< Message text content
    QString toolName;   ///< Tool name (for role="tool" or assistant tool-call)
    QString toolCallId; ///< Tool call ID (for role="tool" response)
};

/// @brief Describes a tool that the LLM can invoke via tool-calling.
struct AgentTool {
    QString name;           ///< Tool function name (e.g. "add_waypoint")
    QString description;    ///< Human-readable description of what the tool does
    QString parametersJson; ///< JSON schema string for tool parameters (OpenAI-compatible)
};

/// @brief Qt wrapper around llama.cpp for local LLM inference with streaming.
///
/// LLMEngine manages the lifecycle of a llama.cpp model, context, sampler chain,
/// and chat templates. It exposes load/unload and completion operations as
/// Q_INVOKABLE methods and emits Qt signals for state changes, streaming tokens,
/// and tool call detection.
///
/// The inference loop runs on a background QThread to avoid blocking the UI.
/// Call moveToThread() with a long-lived QThread before calling startCompletion().
class LLMEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoaded READ isLoaded NOTIFY isLoadedChanged)
    Q_PROPERTY(bool isGenerating READ isGenerating NOTIFY isGeneratingChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(int contextLength READ contextLength WRITE setContextLength NOTIFY contextLengthChanged)
    Q_PROPERTY(int gpuLayers READ gpuLayers WRITE setGpuLayers NOTIFY gpuLayersChanged)
    Q_PROPERTY(int threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(double topP READ topP WRITE setTopP NOTIFY topPChanged)

public:
    explicit LLMEngine(QObject* parent = nullptr);
    virtual ~LLMEngine();

    bool isLoaded() const { return m_isLoaded; }
    bool isGenerating() const { return m_isGenerating; }
    QString modelPath() const { return m_modelPath; }
    int contextLength() const { return m_contextLength; }
    int gpuLayers() const { return m_gpuLayers; }
    int threadCount() const { return m_threadCount; }
    double temperature() const { return m_temperature; }
    double topP() const { return m_topP; }

    void setModelPath(const QString& path);
    void setContextLength(int n);
    void setGpuLayers(int n);
    void setThreadCount(int n);
    void setTemperature(double t);
    void setTopP(double p);

    /// Load the GGUF model from modelPath(). Creates the llama context and
    /// initializes chat templates for Gemma 4 tool-calling support.
    /// Returns true on success, false on failure (emits loadFailed).
    Q_INVOKABLE virtual bool loadModel();

    /// Unload the current model and free all llama.cpp resources.
    Q_INVOKABLE virtual void unloadModel();

    /// Register tools that the LLM can invoke during completion.
    /// Tools are passed to the chat template so the model knows what's available.
    /// Call before startCompletion(). Replaces any previously registered tools.
    Q_INVOKABLE virtual void setTools(const QList<AgentTool>& tools);

    /// Remove all registered tools.
    Q_INVOKABLE virtual void clearTools();

    /// Start a completion cycle with the given message history.
    /// Runs the decode/sample loop on the engine's thread, emitting
    /// tokenGenerated for each token and generationComplete when done.
    /// If a tool call is detected, emits toolCallDetected and stops.
    Q_INVOKABLE virtual void startCompletion(const QList<ChatMessage>& messages);

    /// Cancel an in-progress completion.
    /// Thread-safe: can be called from any thread.
    Q_INVOKABLE virtual void cancelCompletion();

signals:
    void isLoadedChanged();
    void isGeneratingChanged();
    void modelPathChanged();
    void contextLengthChanged();
    void gpuLayersChanged();
    void threadCountChanged();
    void temperatureChanged();
    void topPChanged();
    void loadFailed(const QString& error);

    /// Emitted for each generated token (streaming to UI).
    void tokenGenerated(const QString& token);

    /// Emitted when the full generation is complete.
    void generationComplete(const QString& fullText);

    /// Emitted when a tool call is detected in the model output.
    void toolCallDetected(const QString& toolName, const QString& arguments);

private:
    /// Internal: runs the decode/sample loop. Called on the engine's thread.
    void runCompletion(const QList<ChatMessage>& messages);

    /// Internal: reset the sampler chain. Call only on engine's thread.
    void resetSampler();

    void setIsGenerating(bool generating);

    bool m_isLoaded = false;
    bool m_isGenerating = false;
    QString m_modelPath;
    int m_contextLength = 4096;
    int m_gpuLayers = 0;
    int m_threadCount = 0;      // 0 = auto-detect
    double m_temperature = 0.7;
    double m_topP = 0.9;

    // Registered tools for tool-calling support
    QList<AgentTool> m_tools;

    // Owned llama.cpp objects (raw pointers — freed in destructor/unloadModel)
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    llama_sampler* m_sampler = nullptr;
    common_chat_templates* m_chatTemplates = nullptr;

    // Chat format from the last template application (used for parsing tool calls)
    common_chat_params m_lastChatParams;

    // Cancellation flag — atomic for thread-safe signaling
    std::atomic<bool> m_cancelled{false};
};

Q_DECLARE_METATYPE(ChatMessage)
Q_DECLARE_METATYPE(AgentTool)