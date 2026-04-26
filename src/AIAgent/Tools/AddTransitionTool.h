#pragma once

#include "../AgentToolBase.h"

/// Tool to add a VTOL transition command to the mission plan.
/// Uses insertSimpleMissionItem() then setCommand(MAV_CMD_DO_VTOL_TRANSITION).
/// VTOL-only tool — not available for fixed_wing, multi_rotor, or ground.
class AddTransitionTool : public AgentToolBase {
public:
    explicit AddTransitionTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_transition"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;  // Available in both modes
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        // VTOL transition is ONLY for VTOL vehicles
        return vehicleType == QStringLiteral("vtol");
    }
};