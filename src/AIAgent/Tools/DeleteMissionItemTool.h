#pragma once

#include "../AgentToolBase.h"

/// Tool to delete a mission item from the plan.
/// Bridges LLM tool-calling to MissionController::removeMissionItem().
class DeleteMissionItemTool : public AgentToolBase {
public:
    explicit DeleteMissionItemTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("delete_mission_item"); }
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