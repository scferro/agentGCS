#include "AgentContextSerializer.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

// ---------------------------------------------------------------------------
// Mission state serialization
// ---------------------------------------------------------------------------

QString serializeMissionState()
{
    QJsonObject state;
    state["total_mission_items"] = 0;

    QJsonObject home;
    home["lat"] = 0.0;
    home["lon"] = 0.0;
    home["alt"] = 0.0;
    state["home_position"] = home;

    state["mission_state"] = QJsonObject();

    // TODO (PR 16 — MainWindow integration): When wired into QGC,
    // read from MissionController:
    //   auto* visualItems = MissionController::visualItems();
    //   int count = visualItems->count();
    //   for (int i = 0; i < count; i++) { ... }
    //   state["home_position"] = MissionController::plannedHomePosition();
    //
    // Each VisualMissionItem gets serialized as:
    //   { "type": "waypoint"|"takeoff"|...,
    //     "altitude": "50 m",
    //     "position": "lat/lon (47.397742, 8.545594)" }

    return QJsonDocument(state).toJson(QJsonDocument::Compact);
}

// ---------------------------------------------------------------------------
// Vehicle state serialization
// ---------------------------------------------------------------------------

QString serializeVehicleState()
{
    QJsonObject state;
    state["lat"] = 0.0;
    state["lon"] = 0.0;
    state["alt"] = 0.0;
    state["armed"] = false;
    state["flight_mode"] = "unknown";
    state["battery_percent"] = -1;
    state["gps_lock"] = false;
    state["heading"] = -1;

    // TODO (PR 16 — MainWindow integration): When wired into QGC,
    // read from Vehicle:
    //   state["lat"] = vehicle->latitude();
    //   state["lon"] = vehicle->longitude();
    //   state["alt"] = vehicle->altitudeAMSL()->rawValue().toDouble();
    //   state["armed"] = vehicle->armed();
    //   state["flight_mode"] = vehicle->flightMode();
    //   auto* batteries = vehicle->batteryFactGroupListModel();
    //   state["battery_percent"] = firstBattery->percentRemaining()->rawValue().toFloat();

    return QJsonDocument(state).toJson(QJsonDocument::Compact);
}

// ---------------------------------------------------------------------------
// Context message builders (wrapping JSON in a user-role message)
// ---------------------------------------------------------------------------

QString missionContextMessage()
{
    return QString("Current mission state:\n%1").arg(serializeMissionState());
}

QString vehicleContextMessage()
{
    return QString("Current vehicle state:\n%1").arg(serializeVehicleState());
}