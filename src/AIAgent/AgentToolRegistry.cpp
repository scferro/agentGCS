#include "AgentToolRegistry.h"

AgentToolRegistry::AgentToolRegistry(QObject* parent)
    : QObject(parent)
{
}

void AgentToolRegistry::registerTool(AgentToolBase* tool)
{
    if (tool && !m_tools.contains(tool)) {
        m_tools.append(tool);
    }
}

QList<AgentToolBase*> AgentToolRegistry::getToolsForMode(const QString& mode, const QString& vehicleType) const
{
    QList<AgentToolBase*> result;
    for (auto* tool : m_tools) {
        if (!tool->availableInMode(mode)) {
            continue;
        }
        if (!tool->availableForVehicle(vehicleType)) {
            continue;
        }
        result.append(tool);
    }
    return result;
}

QJsonArray AgentToolRegistry::getToolDefinitions(const QString& mode, const QString& vehicleType) const
{
    QJsonArray defs;
    for (auto* tool : getToolsForMode(mode, vehicleType)) {
        defs.append(tool->toolDefinition());
    }
    return defs;
}

AgentToolBase* AgentToolRegistry::getToolByName(const QString& name) const
{
    for (auto* tool : m_tools) {
        if (tool->name() == name) {
            return tool;
        }
    }
    return nullptr;
}