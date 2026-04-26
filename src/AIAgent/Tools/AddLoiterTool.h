#pragma once

#include "../AgentToolBase.h"

/// Tool to add a loiter/orbit pattern to the mission plan.
/// QGC has no dedicated insertLoiter — we use insertSimpleMissionItem()
/// then setCommand(MAV_CMD_NAV_LOITER_TURNS).
class AddLoiterTool : public AgentToolBase {
public:
    explicit AddLoiterTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_loiter"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;  // Available in both modes
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        // Ground vehicles cannot loiter/orbit
        return vehicleType != QStringLiteral("ground");
    }
};