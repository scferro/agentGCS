#pragma once

#include "../AgentToolBase.h"

/// Tool to command the vehicle to take off to a specified altitude in guided mode.
/// Bridges LLM tool-calling to Vehicle::guidedModeTakeoff().
class GuidedTakeoffTool : public AgentToolBase {
public:
    explicit GuidedTakeoffTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("guided_takeoff"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        return mode == QStringLiteral("command");
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        return vehicleType == QStringLiteral("fixed_wing") || vehicleType == QStringLiteral("vtol");
    }
};