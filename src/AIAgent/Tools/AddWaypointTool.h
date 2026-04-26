#pragma once

#include "../AgentToolBase.h"

/// Tool to add a waypoint navigation item to the mission plan.
/// Bridges LLM tool-calling to MissionController::insertSimpleMissionItem().
class AddWaypointTool : public AgentToolBase {
public:
    explicit AddWaypointTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_waypoint"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;  // Available in both mission and command modes
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        Q_UNUSED(vehicleType);
        return true;  // All vehicle types can have waypoints
    }
};