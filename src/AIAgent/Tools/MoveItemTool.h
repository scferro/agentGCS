#pragma once

#include "../AgentToolBase.h"

/// Tool to move a mission item to a new GPS coordinate.
/// Bridges LLM tool-calling to VisualMissionItem::setCoordinate().
class MoveItemTool : public AgentToolBase {
public:
    explicit MoveItemTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("move_item"); }
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