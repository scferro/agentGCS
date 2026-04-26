#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtCore/QList>
#include <QJsonObject>

#include "LLMEngine.h"
#include "AgentToolRegistry.h"

/// @brief Orchestrates the ReAct (Reason+Act) loop for the AI agent.
///
/// AgentController manages the conversation with the LLM, detects tool calls
/// in the model output, stages proposed actions for user approval, and
/// executes approved actions. The flow is:
///
///   User sends message
///     → Append to chat history
///     → Inject system prompt + mission/vehicle context
///     → Set available tools from registry for current mode/vehicle
///     → Call LLM with all messages
///     → LLM responds:
///       Case A: Text only → display to user, done
///       Case B: Tool call →
///         → Stage action for approval (with description of what it does)
///         → Wait for user approval
///         → If approved: execute tool, append tool result to messages
///         → If rejected: append "Action rejected by user" to messages
///         → Re-call LLM with tool result → repeat loop
///
/// All LLM interaction happens on a background QThread via LLMEngine.
class AgentController : public QObject {
    Q_OBJECT
    Q_PROPERTY(LLMEngine* llmEngine READ llmEngine CONSTANT)
    Q_PROPERTY(AgentToolRegistry* toolRegistry READ toolRegistry CONSTANT)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString vehicleType READ vehicleType WRITE setVehicleType NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(QVariantList pendingActions READ pendingActions NOTIFY pendingActionsChanged)
    Q_PROPERTY(QVariantList chatHistory READ chatHistory NOTIFY chatHistoryChanged)

public:
    /// Construct with optional external LLMEngine (for testing with MockLLMEngine).
    /// If nullptr, creates a real LLMEngine internally on a background thread.
    explicit AgentController(LLMEngine* engine = nullptr, QObject* parent = nullptr);

    LLMEngine* llmEngine() const { return m_llmEngine; }
    AgentToolRegistry* toolRegistry() const { return m_toolRegistry; }
    QString mode() const { return m_mode; }
    QString vehicleType() const { return m_vehicleType; }
    bool isProcessing() const { return m_isProcessing; }
    QVariantList pendingActions() const { return m_pendingActions; }
    QVariantList chatHistory() const { return m_chatHistory; }

    void setMode(const QString& mode);
    void setVehicleType(const QString& vehicleType);

    /// Send a user message and start the ReAct loop.
    Q_INVOKABLE void sendMessage(const QString& text);

    /// Approve a pending action by index. Executes the tool and
    /// continues the ReAct loop with the tool result.
    Q_INVOKABLE void approveAction(int actionIndex);

    /// Reject a pending action by index. Appends a rejection message
    /// and continues the ReAct loop.
    Q_INVOKABLE void rejectAction(int actionIndex);

    /// Approve all pending actions in order.
    Q_INVOKABLE void approveAllActions();

    /// Clear chat history and pending actions.
    Q_INVOKABLE void clearChat();

    /// Set mode based on current view: "plan" → mission mode, "fly" → command mode.
    Q_INVOKABLE void setModeForView(const QString& view);

signals:
    void modeChanged();
    void vehicleTypeChanged();
    void isProcessingChanged();
    void pendingActionsChanged();
    void chatHistoryChanged();

    /// Emitted when the assistant produces a text response (final or intermediate).
    void assistantMessage(const QString& text);

    /// Emitted when a tool call is detected and staged for user approval.
    void actionRequiresApproval(const QString& actionSummary);

private slots:
    void onTokenGenerated(const QString& token);
    void onToolCallDetected(const QString& toolName, const QString& arguments);
    void onGenerationComplete(const QString& fullText);

private:
    /// Run the next iteration of the ReAct loop: set tools, call LLM.
    void runReActLoop();

    /// Stage a tool call as a pending action awaiting user approval.
    void stageActionForApproval(const QString& toolName, const QJsonObject& args, const QString& description);

    /// Execute an approved action by running the tool and appending the result.
    void executeApprovedAction(int actionIndex);

    /// Append a message to chat history and emit chatHistoryChanged.
    void appendChatMessage(const QString& role, const QString& content,
                           const QString& toolName = QString(),
                           const QString& toolCallId = QString());

    /// Set isProcessing flag and emit changed signal.
    void setIsProcessing(bool processing);

    /// Build and append the system prompt for the current mode.
    void injectSystemPrompt();

    /// Append current mission state as a user context message (mission mode).
    void injectMissionState();

    /// Append current vehicle state as a user context message (command mode).
    void injectVehicleState();

    /// Return a unique tool call ID (incrementing counter).
    QString nextToolCallId();

    /// Remove a pending action by index.
    void removePendingAction(int actionIndex);

    LLMEngine* m_llmEngine;
    AgentToolRegistry* m_toolRegistry;
    bool m_ownsEngine = false;       ///< True if we created the engine (and its thread)
    QString m_mode = "mission";       ///< "mission" or "command"
    QString m_vehicleType = "multicopter";
    bool m_isProcessing = false;
    QVariantList m_pendingActions;    ///< Actions awaiting user approval
    QVariantList m_chatHistory;       ///< List of {role, content} dicts for QML
    QList<ChatMessage> m_messages;    ///< Full message list for LLM context

    QString m_accumulatedText;        ///< Current generation text accumulator
    int m_toolCallCounter = 0;        ///< Tool call ID counter
    int m_reActIterations = 0;        ///< Current ReAct loop iteration count
    static constexpr int kMaxReActIterations = 10; ///< Safety limit
};