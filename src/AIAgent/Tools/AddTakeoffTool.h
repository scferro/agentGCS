#pragma once

#include "../AgentToolBase.h"

/// Tool to add a takeoff command to the mission plan.
/// Bridges LLM tool-calling to MissionController::insertTakeoffItem().
class AddTakeoffTool : public AgentToolBase {
public:
    explicit AddTakeoffTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_takeoff"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;  // Available in both modes
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        // Takeoff item explicitly needed for fixed_wing and vtol
        // Multirotor uses simpler takeoff that QGC auto-handles via landing/RTL
        return vehicleType == QStringLiteral("fixed_wing") || vehicleType == QStringLiteral("vtol");
    }
};