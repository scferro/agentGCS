#pragma once

#include "../AgentToolBase.h"

/// Tool to update properties of an existing mission item.
/// Bridges LLM tool-calling to SimpleMissionItem Fact setters.
class UpdateMissionItemTool : public AgentToolBase {
public:
    explicit UpdateMissionItemTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("update_mission_item"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        return mode == QStringLiteral("mission");
    }
    bool availableForVehicle(const QString& /*vehicleType*/) const override {
        return true;
    }
};