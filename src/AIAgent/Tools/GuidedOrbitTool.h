#pragma once

#include "../AgentToolBase.h"

/// Tool to command the vehicle to orbit around a specific GPS coordinate.
/// Bridges LLM tool-calling to Vehicle::guidedModeOrbit().
class GuidedOrbitTool : public AgentToolBase {
public:
    explicit GuidedOrbitTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("guided_orbit"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        return mode == QStringLiteral("command");
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        return vehicleType == QStringLiteral("rotor")
            || vehicleType == QStringLiteral("vtol")
            || vehicleType == QStringLiteral("multi_rotor");
    }
};