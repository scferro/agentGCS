#pragma once

#include "../AgentToolBase.h"

/// Tool to add a Return-To-Launch command to the mission plan.
/// QGC has no dedicated insertRTL — we use insertSimpleMissionItem()
/// then setCommand(MAV_CMD_NAV_RETURN_TO_LAUNCH).
class AddRTLTool : public AgentToolBase {
public:
    explicit AddRTLTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_rtl"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;  // Available in both modes
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        Q_UNUSED(vehicleType);
        return true;  // All vehicle types support RTL
    }
};