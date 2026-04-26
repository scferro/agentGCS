#pragma once

#include "LLMEngine.h"

#include <QtCore/QTimer>
#include <QtCore/QJsonDocument>

/// @brief Mock LLMEngine for testing AgentController's ReAct loop.
///
/// Instead of running actual inference, this mock returns scripted responses
/// via queued timer callbacks. Tests can configure the response sequence
/// before starting the loop.
///
/// Usage:
///   MockLLMEngine mock;
///   mock.addScriptedResponse("text_only",  "I'll help with that.");
///   mock.addScriptedResponse("tool_call",  "add_waypoint", R"({"lat":47.5,"lon":-122.3,"alt":50})");
///   mock.addScriptedResponse("text_only",  "Done! Waypoint added.");
///
/// Each call to startCompletion pops the next scripted response and emits
/// the appropriate signal(s) after a brief delay (to simulate async inference
/// and to allow the Qt event loop to process intermediate state).
class MockLLMEngine : public LLMEngine {
    Q_OBJECT

public:
    /// Type of scripted response.
    enum ResponseType {
        TextOnly,       ///< Emits generationComplete with text
        ToolCall,       ///< Emits toolCallDetected then generationComplete
        Empty           ///< Emits generationComplete with empty string
    };

    explicit MockLLMEngine(QObject* parent = nullptr)
        : LLMEngine(parent)
    {}

    /// Add a scripted text-only response.
    void addScriptedTextResponse(const QString& text)
    {
        ScriptedResponse r;
        r.type = TextOnly;
        r.text = text;
        m_responses.append(r);
    }

    /// Add a scripted tool-call response.
    void addScriptedToolCall(const QString& toolName, const QString& arguments)
    {
        ScriptedResponse r;
        r.type = ToolCall;
        r.toolName = toolName;
        r.arguments = arguments;
        m_responses.append(r);
    }

    /// Add a scripted empty response.
    void addScriptedEmptyResponse()
    {
        ScriptedResponse r;
        r.type = Empty;
        m_responses.append(r);
    }

    /// How many startCompletion calls have been made.
    int completionCount() const { return m_completionCount; }

    /// Get all tools that were registered via setTools() (for verification).
    QList<AgentTool> lastSetTools() const { return m_lastTools; }

    /// Get the messages from the last startCompletion() call.
    QList<ChatMessage> lastMessages() const { return m_lastMessages; }

    // Override to capture tools and messages, then emit scripted response.
    void startCompletion(const QList<ChatMessage>& messages) override
    {
        m_lastMessages = messages;
        m_completionCount++;

        if (m_responses.isEmpty()) {
            // No more scripted responses — emit empty completion.
            QTimer::singleShot(0, this, [this]() {
                emit generationComplete("");
            });
            return;
        }

        ScriptedResponse r = m_responses.takeFirst();

        QTimer::singleShot(0, this, [this, r]() {
            if (r.type == ToolCall) {
                emit toolCallDetected(r.toolName, r.arguments);
                emit toolCallDetectedStreaming(r.toolName, r.arguments);
                emit generationComplete("");
            } else if (r.type == TextOnly) {
                // Simulate token streaming.
                emit tokenGenerated(r.text);
                emit generationComplete(r.text);
            } else {
                emit generationComplete("");
            }
        });
    }

    // Override setTools to capture what tools were registered.
    void setTools(const QList<AgentTool>& tools) override
    {
        m_lastTools = tools;
        LLMEngine::setTools(tools);
    }

    // Override no-ops that we don't need for testing.
    bool loadModel() override { return true; }
    void unloadModel() override {}
    void clearTools() override { m_lastTools.clear(); LLMEngine::clearTools(); }
    void cancelCompletion() override {}

private:
    struct ScriptedResponse {
        ResponseType type = TextOnly;
        QString text;
        QString toolName;
        QString arguments;
    };

    QList<ScriptedResponse> m_responses;
    QList<AgentTool> m_lastTools;
    QList<ChatMessage> m_lastMessages;
    int m_completionCount = 0;
};