#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

#include "AgentToolBase.h"

/// Registry of AI agent tools. Mirrors mav-agent's get_tools_for_mode() /
/// _filter_tools_for_vehicle() pattern. Tools are registered at startup and
/// filtered at query time based on the current flight mode and vehicle type.
///
/// Mode filtering logic (from mav-agent):
/// - "command" mode: only "add" tools (add_waypoint, add_takeoff, add_loiter,
///   add_survey, add_rtl, add_land, add_transition)
/// - "mission" mode: all tools (add + edit)
///
/// Vehicle filtering:
/// - "ground"/"rover" vehicles exclude loiter tools
/// - "fixed_wing" vehicles include all tools
/// - "multi_rotor" vehicles include all tools
class AgentToolRegistry : public QObject {
    Q_OBJECT
public:
    explicit AgentToolRegistry(QObject* parent = nullptr);

    /// Register a tool. Ownership is NOT transferred (tools are typically static instances).
    void registerTool(AgentToolBase* tool);

    /// Get all tools matching the given mode and vehicle type.
    QList<AgentToolBase*> getToolsForMode(const QString& mode, const QString& vehicleType) const;

    /// Get tool definitions as a QJsonArray suitable for LLM function calling.
    QJsonArray getToolDefinitions(const QString& mode, const QString& vehicleType) const;

    /// Get a tool by name, or nullptr if not found.
    AgentToolBase* getToolByName(const QString& name) const;

    /// Get all registered tools (unfiltered).
    const QList<AgentToolBase*>& allTools() const { return m_tools; }

private:
    QList<AgentToolBase*> m_tools;
};