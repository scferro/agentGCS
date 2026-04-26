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
        Q_UNUSED(mode);
        return true;  // Available in both modes
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        Q_UNUSED(vehicleType);
        return true;  // All vehicle types support survey
    }
};