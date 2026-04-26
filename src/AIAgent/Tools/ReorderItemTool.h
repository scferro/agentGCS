#pragma once

#include "../AgentToolBase.h"

/// Tool to change the order of mission items by moving an item from one position to another.
/// Bridges LLM tool-calling to MissionController visual item list manipulation.
class ReorderItemTool : public AgentToolBase {
public:
    explicit ReorderItemTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("reorder_item"); }
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