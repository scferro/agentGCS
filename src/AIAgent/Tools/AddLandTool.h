#pragma once

#include "../AgentToolBase.h"

/// Tool to add a land command to the mission plan.
/// Bridges LLM tool-calling to MissionController::insertLandItem().
/// QGC's insertLandItem() auto-routes by vehicle type:
///   fixed-wing → FixedWingLandingComplexItem
///   VTOL       → VTOLLandingComplexItem
///   multirotor → MAV_CMD_NAV_RETURN_TO_LAUNCH (RTL)
class AddLandTool : public AgentToolBase {
public:
    explicit AddLandTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_land"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;  // Available in both modes
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        Q_UNUSED(vehicleType);
        return true;  // All vehicle types support landing
    }
};