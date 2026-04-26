#pragma once

#include "../AgentToolBase.h"

/// Tool to add a Survey complex mission item to the plan.
/// Bridges LLM tool-calling to MissionController::insertComplexMissionItem("Survey", ...).
class AddSurveyTool : public AgentToolBase {
public:
    explicit AddSurveyTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_survey"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        return mode == "mission";  // Survey requires complex mission item setup
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        // Fixed-wing and multi-rotor only (per mav-agent filtering)
        return vehicleType == "fixed_wing" || vehicleType == "rotor";
    }
};