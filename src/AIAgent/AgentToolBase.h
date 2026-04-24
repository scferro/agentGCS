#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class Vehicle;
class PlanMasterController;

/// Abstract base class for AI agent tools. Each tool declares its name,
/// description, JSON-schema parameters, and execution logic. Tools can
/// also declare availability constraints by mode (command/mission) and
/// vehicle type (fixed_wing/multi_rotor/ground/rover).
class AgentToolBase : public QObject {
    Q_OBJECT
public:
    explicit AgentToolBase(QObject* parent = nullptr) : QObject(parent) {}

    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QJsonObject parameters() const = 0;   // JSON schema object
    virtual QString execute(const QJsonObject& args) = 0;  // Returns result string

    virtual bool availableInMode(const QString& mode) const { return true; }
    virtual bool availableForVehicle(const QString& vehicleType) const { return true; }

    /// Convenience: returns the full tool definition as a JSON object suitable for LLM function calling
    QJsonObject toolDefinition() const;

protected:
    Vehicle* activeVehicle() const;
    PlanMasterController* planController() const;
};