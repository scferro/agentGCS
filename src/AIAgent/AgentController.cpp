#include "AgentController.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaObject>
#include <QtCore/QThread>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

AgentController::AgentController(LLMEngine* engine, QObject* parent)
    : QObject(parent)
    , m_llmEngine(engine ? engine : new LLMEngine(this))
    , m_toolRegistry(new AgentToolRegistry(this))
    , m_ownsEngine(!engine)
{
    // If we created the engine, move it to a background thread.
    if (m_ownsEngine) {
        auto* llmThread = new QThread(this);
        m_llmEngine->moveToThread(llmThread);
        llmThread->start();
    }

    // Wire LLM engine signals to AgentController slots.
    // Use queued connections because the engine may run on a different thread.
    connect(m_llmEngine, &LLMEngine::tokenGenerated,
            this, &AgentController::onTokenGenerated,
            Qt::QueuedConnection);
    connect(m_llmEngine, &LLMEngine::toolCallDetected,
            this, &AgentController::onToolCallDetected,
            Qt::QueuedConnection);
    connect(m_llmEngine, &LLMEngine::generationComplete,
            this, &AgentController::onGenerationComplete,
            Qt::QueuedConnection);
}

// ---------------------------------------------------------------------------
// Property accessors
// ---------------------------------------------------------------------------

void AgentController::setMode(const QString& mode)
{
    if (m_mode == mode) return;
    m_mode = mode;
    emit modeChanged();

    // Mode change resets pending actions (they belong to the old mode's context).
    if (!m_pendingActions.isEmpty()) {
        m_pendingActions.clear();
        emit pendingActionsChanged();
    }
}

void AgentController::setVehicleType(const QString& vehicleType)
{
    if (m_vehicleType == vehicleType) return;
    m_vehicleType = vehicleType;
    emit vehicleTypeChanged();
}

// ---------------------------------------------------------------------------
// Q_INVOKABLE public API
// ---------------------------------------------------------------------------

void AgentController::sendMessage(const QString& text)
{
    if (m_isProcessing) {
        qWarning() << "AgentController::sendMessage: already processing, ignoring";
        return;
    }

    // Inject system prompt before the first user message if needed.
    bool hasSystemPrompt = false;
    for (const auto& msg : m_messages) {
        if (msg.role == "system") {
            hasSystemPrompt = true;
            break;
        }
    }
    if (!hasSystemPrompt) {
        injectSystemPrompt();
    }

    appendChatMessage("user", text);
    m_reActIterations = 0;
    runReActLoop();
}

void AgentController::approveAction(int actionIndex)
{
    if (actionIndex < 0 || actionIndex >= m_pendingActions.size()) {
        qWarning() << "AgentController::approveAction: index out of range:" << actionIndex;
        return;
    }
    executeApprovedAction(actionIndex);
}

void AgentController::rejectAction(int actionIndex)
{
    if (actionIndex < 0 || actionIndex >= m_pendingActions.size()) {
        qWarning() << "AgentController::rejectAction: index out of range:" << actionIndex;
        return;
    }

    QVariantMap action = m_pendingActions[actionIndex].toMap();
    QString toolName = action["toolName"].toString();
    QString callId = action["toolCallId"].toString();

    removePendingAction(actionIndex);

    // Append rejection as tool result so the LLM knows the action was denied.
    appendChatMessage("tool", "Action rejected by user.", toolName, callId);

    // Continue the ReAct loop so the LLM can respond to the rejection.
    runReActLoop();
}

void AgentController::approveAllActions()
{
    while (!m_pendingActions.isEmpty()) {
        executeApprovedAction(0);
    }
}

void AgentController::clearChat()
{
    m_chatHistory.clear();
    m_messages.clear();
    m_pendingActions.clear();
    m_accumulatedText.clear();
    m_reActIterations = 0;
    emit chatHistoryChanged();
    emit pendingActionsChanged();
}

void AgentController::setModeForView(const QString& view)
{
    if (view == "plan") {
        setMode("mission");
    } else if (view == "fly") {
        setMode("command");
    }
}

// ---------------------------------------------------------------------------
// LLM signal handlers
// ---------------------------------------------------------------------------

void AgentController::onTokenGenerated(const QString& token)
{
    m_accumulatedText += token;
}

void AgentController::onToolCallDetected(const QString& toolName, const QString& arguments)
{
    // Parse the arguments JSON.
    QJsonObject argsObj;
    QJsonDocument argsDoc = QJsonDocument::fromJson(arguments.toUtf8());
    if (argsDoc.isObject()) {
        argsObj = argsDoc.object();
    } else {
        qWarning() << "AgentController: tool call arguments not valid JSON, using empty object";
    }

    // Build a human-readable description of the proposed action.
    QString description = QString("Tool: %1\nArguments: %2")
                              .arg(toolName, arguments);

    // Append the assistant's tool-call message to chat history (for LLM context).
    QString callId = nextToolCallId();
    appendChatMessage("assistant", m_accumulatedText, toolName, callId);

    m_accumulatedText.clear();

    // Stage the action for user approval.
    stageActionForApproval(toolName, argsObj, description);

    // Emit signal so UI can show the approval card.
    emit actionRequiresApproval(description);
}

void AgentController::onGenerationComplete(const QString& fullText)
{
    // If we already processed a tool call during this generation,
    // the assistant message was already appended in onToolCallDetected.
    // Only append a final text response here if this was a text-only response.
    if (m_pendingActions.isEmpty() && !fullText.isEmpty()) {
        appendChatMessage("assistant", fullText);
        emit assistantMessage(fullText);
    }

    m_accumulatedText.clear();
    setIsProcessing(false);
}

// ---------------------------------------------------------------------------
// ReAct loop
// ---------------------------------------------------------------------------

void AgentController::runReActLoop()
{
    m_reActIterations++;
    if (m_reActIterations > kMaxReActIterations) {
        appendChatMessage("assistant", "I've reached the maximum number of reasoning steps. Please rephrase your request.");
        emit assistantMessage(m_chatHistory.last().toMap()["content"].toString());
        setIsProcessing(false);
        return;
    }

    setIsProcessing(true);

    // Set available tools from registry for current mode and vehicle type.
    QList<AgentToolBase*> tools = m_toolRegistry->getToolsForMode(m_mode, m_vehicleType);
    QList<AgentTool> toolDefs;
    for (AgentToolBase* tool : tools) {
        AgentTool def;
        def.name = tool->name();
        def.description = tool->description();
        QJsonObject paramsObj = tool->parameters();
        def.parametersJson = QJsonDocument(paramsObj).toJson(QJsonDocument::Compact);
        toolDefs.append(def);
    }

    // Register tools with the LLM engine (must happen on engine's thread).
    QMetaObject::invokeMethod(m_llmEngine, [this, toolDefs]() {
        m_llmEngine->setTools(toolDefs);

        // Start completion on the engine's thread.
        m_llmEngine->startCompletion(m_messages);
    });
}

// ---------------------------------------------------------------------------
// Action staging & execution
// ---------------------------------------------------------------------------

void AgentController::stageActionForApproval(const QString& toolName,
                                              const QJsonObject& args,
                                              const QString& description)
{
    QVariantMap action;
    action["toolName"] = toolName;
    action["arguments"] = args.toVariantMap();
    action["description"] = description;
    action["toolCallId"] = nextToolCallId();

    m_pendingActions.append(action);
    emit pendingActionsChanged();
}

void AgentController::executeApprovedAction(int actionIndex)
{
    QVariantMap action = m_pendingActions[actionIndex].toMap();
    QString toolName = action["toolName"].toString();
    QJsonObject args = QJsonObject::fromVariantMap(action["arguments"].toMap());
    QString callId = action["toolCallId"].toString();

    removePendingAction(actionIndex);

    // Find the tool in the registry.
    AgentToolBase* tool = m_toolRegistry->getToolByName(toolName);
    QString result;
    if (tool) {
        result = tool->execute(args);
    } else {
        result = QString("Error: tool '%1' not found in registry.").arg(toolName);
        qWarning() << "AgentController: tool not found:" << toolName;
    }

    // Append tool result as a tool-role message.
    appendChatMessage("tool", result, toolName, callId);

    // Continue the ReAct loop with the tool result.
    runReActLoop();
}

// ---------------------------------------------------------------------------
// Chat history management
// ---------------------------------------------------------------------------

void AgentController::appendChatMessage(const QString& role,
                                         const QString& content,
                                         const QString& toolName,
                                         const QString& toolCallId)
{
    ChatMessage msg;
    msg.role = role;
    msg.content = content;
    msg.toolName = toolName;
    msg.toolCallId = toolCallId;
    m_messages.append(msg);

    // Mirror into QML-friendly QVariantList.
    QVariantMap map;
    map["role"] = role;
    map["content"] = content;
    if (!toolName.isEmpty()) map["toolName"] = toolName;
    if (!toolCallId.isEmpty()) map["toolCallId"] = toolCallId;
    m_chatHistory.append(map);
    emit chatHistoryChanged();
}

// ---------------------------------------------------------------------------
// System prompt & context injection (stubs — PR 13 fills these in)
// ---------------------------------------------------------------------------

void AgentController::injectSystemPrompt()
{
    // System prompt for the current mode. These are intentionally basic
    // stubs — PR 13 (Task 3.2) ports the full mav-agent system prompts.
    QString systemPrompt;
    if (m_mode == "mission") {
        systemPrompt = "You are an AI assistant for drone mission planning. "
                       "You help users create and edit flight plans. "
                       "You have access to tools for adding, editing, and removing mission items. "
                       "Always confirm actions that modify the mission plan.";
    } else {
        systemPrompt = "You are an AI assistant for real-time drone control. "
                       "You help users with live vehicle operations. "
                       "You have access to guided action tools for controlling the vehicle. "
                       "Always confirm actions before sending commands to the vehicle.";
    }
    appendChatMessage("system", systemPrompt);
}

void AgentController::injectMissionState()
{
    // Stub — PR 13 implements real MissionController state serialization.
    appendChatMessage("user", "[Mission context: no active mission]");
}

void AgentController::injectVehicleState()
{
    // Stub — PR 13 implements real Vehicle state serialization.
    appendChatMessage("user", "[Vehicle context: no connected vehicle]");
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QString AgentController::nextToolCallId()
{
    return QString("call_%1").arg(m_toolCallCounter++);
}

void AgentController::removePendingAction(int actionIndex)
{
    m_pendingActions.removeAt(actionIndex);
    emit pendingActionsChanged();
}

void AgentController::setIsProcessing(bool processing)
{
    if (m_isProcessing == processing) return;
    m_isProcessing = processing;
    emit isProcessingChanged();
}