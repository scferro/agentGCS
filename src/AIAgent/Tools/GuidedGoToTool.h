#pragma once

#include "../AgentToolBase.h"

/// Tool to command the vehicle to fly to specific GPS coordinates in guided mode.
/// Bridges LLM tool-calling to Vehicle::guidedModeGotoLocation().
class GuidedGoToTool : public AgentToolBase {
public:
    explicit GuidedGoToTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("guided_goto"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        return mode == QStringLiteral("command");
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        return vehicleType != QStringLiteral("ground") && vehicleType != QStringLiteral("rover");
    }
};