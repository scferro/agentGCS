#pragma once

#include "../AgentToolBase.h"

/// Tool to command the vehicle to return to launch/home position.
/// Bridges LLM tool-calling to Vehicle::guidedModeRTL().
class GuidedRTLTool : public AgentToolBase {
public:
    explicit GuidedRTLTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("guided_rtl"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        return mode == QStringLiteral("command");
    }
    bool availableForVehicle(const QString& /*vehicleType*/) const override {
        return true;  // All vehicle types
    }
};