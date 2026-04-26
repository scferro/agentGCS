#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QString>

/// @brief Serializes mission state and vehicle state into JSON for LLM context.
///
/// These functions produce JSON strings that get injected as user context messages
/// into the LLM conversation, giving the model awareness of the current mission
/// plan and vehicle status.
///
/// In standalone test mode (no QGC deps), these return mock/empty data.
/// When integrated into QGC, they read from MissionController and Vehicle.

/// Serialize current mission state as JSON.
/// Returns: { "total_mission_items": N, "home_position": {lat,lon,alt},
///             "mission_state": { "item_1": {...}, ... } }
QString serializeMissionState();

/// Serialize current vehicle state as JSON.
/// Returns: { "lat": ..., "lon": ..., "alt": ..., "armed": bool,
///             "flight_mode": "...", "battery_percent": N,
///             "gps_lock": bool, "heading": N }
QString serializeVehicleState();

/// Build a user-context message string wrapping the mission state JSON.
QString missionContextMessage();

/// Build a user-context message string wrapping the vehicle state JSON.
QString vehicleContextMessage();