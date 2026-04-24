#include "AgentToolBase.h"

QJsonObject AgentToolBase::toolDefinition() const
{
    QJsonObject def;
    def["name"] = name();
    def["description"] = description();
    def["parameters"] = parameters();
    return def;
}

// activeVehicle() and planController() are stubs — will be wired up
// when AgentController injects the QGC context into each tool.
// For now, tool implementations that need QGC APIs can override
// these or receive the context via setParent/QProperty.

Vehicle* AgentToolBase::activeVehicle() const { return nullptr; }
PlanMasterController* AgentToolBase::planController() const { return nullptr; }