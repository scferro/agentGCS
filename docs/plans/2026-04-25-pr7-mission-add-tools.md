# PR 7 — Mission Add Tools: Detailed Task Plan

**Branch:** `feat/ai-agent-pr7-mission-add-tools` (from `master` with PRs 1-8 merged)

## Current State

✅ `AgentToolBase.h/.cpp` — abstract base with `name()`, `description()`, `parameters()` (QJsonObject), `execute(QJsonObject)`, `availableInMode()`, `availableForVehicle()`, `toolDefinition()`
✅ `AgentToolRegistry.h/.cpp` — `registerTool()`, `getToolsForMode()`, `getToolDefinitions()`, `getToolByName()`
✅ `LLMEngine.h/.cpp` — tool calling support with `AgentTool` struct, `setTools()`, `toolCallDetected` signal
✅ `CMakeLists.txt` — AIAgentModule building with llama.cpp, nlohmann_json, Qt6
✅ All unit tests passing in Docker

## Scope

Port mav-agent's 7 "add" mission tools to C++, each extending `AgentToolBase`. Tools call QGC's `MissionController` APIs directly from C++. JSON schemas mirror mav-agent's Pydantic models (simplified — no MGRS, search_target, or detection_behavior in v1). Human-in-the-loop confirmation is NOT in this PR (PR 12).

### Correct QGC API Names

The master integration plan had incorrect method names. The actual QGC APIs are:

| Tool | QGC Method | Notes |
|------|-----------|-------|
| add_waypoint | `MissionController::insertSimpleMissionItem(coordinate, index)` | Uses `MAV_CMD_NAV_WAYPOINT` |
| add_takeoff | `MissionController::insertTakeoffItem(coordinate, index)` | Handles VTOL vs fixed-wing internally |
| add_land | `MissionController::insertLandItem(coordinate, index)` | FixedWing→complex item, multirotor→`MAV_CMD_NAV_RETURN_TO_LAUNCH`, VTOL→complex item |
| add_loiter | `MissionController::insertSimpleMissionItem(coordinate, index)` then `setCommand(MAV_CMD_NAV_LOITER_TURNS)` | No dedicated insertLoiter — post-create command change |
| add_rtl | `MissionController::insertSimpleMissionItem(coordinate, index)` then `setCommand(MAV_CMD_NAV_RETURN_TO_LAUNCH)` | No dedicated insertRTL |
| add_survey | `MissionController::insertComplexMissionItem("Survey", coordinate, index)` | Complex item |
| add_transition | `MissionController::insertSimpleMissionItem(coordinate, index)` then `setCommand(MAV_CMD_DO_VTOL_TRANSITION)` then set param1 | VTOL only |

---

## Tasks

### Task 7.1: Create Tools directory and AddWaypointTool (reference implementation)

**Objective:** Implement the first and most complex tool as a reference pattern for the remaining 6 tools.

**Files:**
- Create: `src/AIAgent/Tools/AddWaypointTool.h`
- Create: `src/AIAgent/Tools/AddWaypointTool.cpp`
- Create: `src/AIAgent/Tools/CMakeLists.txt`

**Step 1: Create Tools directory structure**

```bash
mkdir -p src/AIAgent/Tools
```

**Step 2: Write AddWaypointTool.h**

```cpp
#pragma once

#include "../AgentToolBase.h"

class AddWaypointTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit AddWaypointTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_waypoint"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        // Available in both mission and command modes
        Q_UNUSED(mode);
        return true;
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        // All vehicle types can have waypoints
        Q_UNUSED(vehicleType);
        return true;
    }
};
```

**Step 3: Write AddWaypointTool.cpp**

```cpp
#include "AddWaypointTool.h"
#include "../AgentToolRegistry.h"
#include "../../MissionManager/MissionController.h"
#include "../../MissionManager/SimpleMissionItem.h"
#include "../../MissionManager/PlanMasterController.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString AddWaypointTool::description() const {
    return QStringLiteral(
        "Add waypoint for drone navigation to specific location. Use when user wants drone to fly to a "
        "location using exact GPS coordinates or relative directions. Creates flight path point where "
        "drone flies to location, flies THROUGH it, then continues to the next mission item. "
        "Specify coordinates OR distance/heading/reference_frame. Do not mix location systems."
    );
}

QJsonObject AddWaypointTool::parameters() const {
    // JSON Schema matching mav-agent's WaypointInput (simplified v1 — no MGRS/search/detection)
    QJsonObject schema;
    schema["type"] = "object";

    QJsonObject props;
    props["coordinates"] = QJsonObject{
        {"type", "string"},
        {"description", "GPS coordinates as 'lat,lon' (e.g., '40.7128,-74.0060'). "
                        "Avoid using unless user provides exact coordinates. "
                        "Prefer distance/heading/reference_frame for more intuitive positioning."}
    };
    props["altitude"] = QJsonObject{
        {"type", "number"},
        {"description", "Flight altitude for this waypoint in meters. "
                        "Specify only if user mentions altitude. Default = 50 meters."}
    };
    props["altitude_units"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"meters", "feet"}},
        {"description", "Unit for altitude value. Default = meters."}
    };
    props["seq"] = QJsonObject{
        {"type", "integer"},
        {"description", "Position to insert waypoint in mission (1-based index). "
                        "The waypoint will be inserted AT this position, shifting existing items down. "
                        "Omit to add at end."}
    };
    // Relative positioning (simplified)
    props["distance"] = QJsonObject{
        {"type", "number"},
        {"description", "Distance value in meters for relative positioning. "
                        "Always use with heading parameter."}
    };
    props["heading"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"north", "northeast", "east", "southeast", "south",
                            "southwest", "west", "northwest"}},
        {"description", "Compass direction for relative positioning. Always use with distance."}
    };
    props["relative_reference_frame"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"origin", "last_waypoint"}},
        {"description", "Reference point for distance: 'origin' (takeoff) or 'last_waypoint'. "
                        "Use 'origin' when user references start/takeoff/here. "
                        "Otherwise assume last_waypoint."}
    };

    schema["properties"] = props;

    QJsonArray required;
    // No required fields — coordinates OR relative positioning can be used
    schema["required"] = required;

    return schema;
}

QString AddWaypointTool::execute(const QJsonObject& args) {
    // 1. Get MissionController from the base class helper
    auto* controller = planController();
    if (!controller) {
        return QStringLiteral("Error: PlanMasterController not available");
    }
    auto* missionController = controller->missionController();
    if (!missionController) {
        return QStringLiteral("Error: MissionController not available");
    }

    // 2. Parse coordinates
    QGeoCoordinate coordinate;
    if (args.contains("coordinates") && !args["coordinates"].toString().isEmpty()) {
        // Parse "lat,lon" format
        QString coordStr = args["coordinates"].toString();
        QStringList parts = coordStr.split(',');
        if (parts.size() == 2) {
            bool ok1, ok2;
            double lat = parts[0].trimmed().toDouble(&ok1);
            double lon = parts[1].trimmed().toDouble(&ok2);
            if (ok1 && ok2) {
                coordinate = QGeoCoordinate(lat, lon);
            }
        }
        if (!coordinate.isValid()) {
            return QStringLiteral("Error: Invalid coordinates format. Use 'lat,lon' (e.g., '40.7128,-74.0060')");
        }
    } else if (args.contains("distance")) {
        // TODO (PR 9+): Resolve relative coordinates using home position or last waypoint
        // For now, require absolute coordinates
        return QStringLiteral("Error: Relative positioning not yet supported. Provide coordinates as 'lat,lon'.");
    } else {
        // Use home position as fallback if no coordinates specified
        auto* vehicle = activeVehicle();
        if (vehicle) {
            coordinate = vehicle->homePosition();
        }
        if (!coordinate.isValid()) {
            return QStringLiteral("Error: No coordinates provided and no home position available");
        }
    }

    // 3. Determine insertion index
    int insertIndex = -1; // -1 = append to end
    if (args.contains("seq")) {
        insertIndex = args["seq"].toInt();
    }

    // 4. Insert waypoint via MissionController
    VisualMissionItem* item = missionController->insertSimpleMissionItem(coordinate, insertIndex);
    if (!item) {
        return QStringLiteral("Error: Failed to insert waypoint");
    }

    // 5. Set altitude if specified
    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem && args.contains("altitude")) {
        double altitude = args["altitude"].toDouble();
        // Convert from feet if specified
        if (args.contains("altitude_units") && args["altitude_units"].toString() == "feet") {
            altitude *= 0.3048; // feet to meters
        }
        simpleItem->altitude()->setRawValue(altitude);
    }

    // 6. Build result message
    int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    QString altMsg = args.contains("altitude")
        ? QString("%1 %2").arg(args["altitude"].toDouble()).arg(args["altitude_units"].toString("meters"))
        : QStringLiteral("default");

    return QStringLiteral("Waypoint added to mission: (%1, %2), Alt=%3 (Item %4)")
        .arg(coordinate.latitude(), 0, 'f', 6)
        .arg(coordinate.longitude(), 0, 'f', 6)
        .arg(altMsg)
        .arg(seqNum);
}
```

**Step 4: Write Tools/CMakeLists.txt**

```cmake
target_sources(AIAgentModule
    PRIVATE
        AddWaypointTool.h
        AddWaypointTool.cpp
)

target_include_directories(AIAgentModule
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}  # Tools/ dir for internal includes
)
```

**Step 5: Add subdirectory to src/AIAgent/CMakeLists.txt**

Append after existing `target_sources` block:

```cmake
add_subdirectory(Tools)
```

**Verification:** Project compiles with `ninja AIAgentModule` in Docker build.

---

### Task 7.2: Create AddTakeoffTool and AddLandTool

**Objective:** Implement takeoff (fixed-wing/VTOL only) and land tools.

**Files:**
- Create: `src/AIAgent/Tools/AddTakeoffTool.h`
- Create: `src/AIAgent/Tools/AddTakeoffTool.cpp`
- Create: `src/AIAgent/Tools/AddLandTool.h`
- Create: `src/AIAgent/Tools/AddLandTool.cpp`

**AddTakeoffTool.h:**

```cpp
#pragma once

#include "../AgentToolBase.h"

class AddTakeoffTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit AddTakeoffTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_takeoff"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;  // Available in both modes
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        // Takeoff item explicitly needed for fixed_wing and vtol
        return vehicleType == "fixed_wing" || vehicleType == "vtol";
    }
};
```

**AddTakeoffTool.cpp key logic:**

```cpp
QString AddTakeoffTool::description() const {
    return QStringLiteral(
        "Add takeoff command to launch drone from ground to flight altitude. "
        "Always inserted as the FIRST mission item. Use when user wants drone to take off, "
        "launch, or lift off, especially when altitude is specified like 'takeoff to 200 feet'."
    );
}

QJsonObject AddTakeoffTool::parameters() const {
    QJsonObject schema;
    schema["type"] = "object";
    QJsonObject props;
    props["altitude"] = QJsonObject{
        {"type", "number"},
        {"description", "Target takeoff altitude in meters. "
                        "DO NOT include unless directly specified by the user. Default = 30 meters."}
    };
    props["altitude_units"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"meters", "feet"}},
        {"description", "Unit for altitude. Default = meters."}
    };
    props["heading"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"north", "northeast", "east", "southeast", "south",
                            "southwest", "west", "northwest"}},
        {"description", "Direction VTOL will point during transition to forward flight. "
                        "Typically into the wind. Use ONLY when direction is specified and vehicle is VTOL."}
    };
    schema["properties"] = props;
    return schema;
}

QString AddTakeoffTool::execute(const QJsonObject& args) {
    auto* controller = planController();
    if (!controller) return "Error: PlanMasterController not available";
    auto* missionController = controller->missionController();
    if (!missionController) return "Error: MissionController not available";

    QGeoCoordinate coordinate;
    auto* vehicle = activeVehicle();
    if (vehicle) coordinate = vehicle->homePosition();

    // Takeoff is always inserted at index 1 (after home position)
    VisualMissionItem* item = missionController->insertTakeoffItem(coordinate, 1);
    if (!item) return "Error: Failed to insert takeoff item";

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem && args.contains("altitude")) {
        double altitude = args["altitude"].toDouble();
        if (args.contains("altitude_units") && args["altitude_units"].toString() == "feet") {
            altitude *= 0.3048;
        }
        simpleItem->altitude()->setRawValue(altitude);
    }

    int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    QString altMsg = args.contains("altitude")
        ? QString("%1 %2").arg(args["altitude"].toDouble()).arg(args["altitude_units"].toString("meters"))
        : "default";

    return QStringLiteral("Takeoff command added to mission, Alt=%1 (Item %2)")
        .arg(altMsg).arg(seqNum);
}
```

**AddLandTool.h:**

```cpp
#pragma once

#include "../AgentToolBase.h"

class AddLandTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit AddLandTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_land"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        Q_UNUSED(vehicleType);
        return true;  // All vehicle types
    }
};
```

**AddLandTool.cpp key logic:**

```cpp
QString AddLandTool::description() const {
    return QStringLiteral(
        "Add land command at specified location or current position. "
        "Always inserted as the LAST mission item. Use when the drone should land. "
        "For multirotor this creates an RTL command; for fixed-wing a landing pattern; "
        "for VTOL a VTOL landing pattern."
    );
}

QJsonObject AddLandTool::parameters() const {
    QJsonObject schema;
    schema["type"] = "object";
    QJsonObject props;
    props["coordinates"] = QJsonObject{
        {"type", "string"},
        {"description", "GPS coordinates as 'lat,lon' for landing location. "
                        "Leave empty for default landing at home position."}
    };
    props["altitude"] = QJsonObject{
        {"type", "number"},
        {"description", "Landing altitude in meters. Default = 0."}
    };
    schema["properties"] = props;
    return schema;
}

QString AddLandTool::execute(const QJsonObject& args) {
    auto* controller = planController();
    if (!controller) return "Error: PlanMasterController not available";
    auto* missionController = controller->missionController();
    if (!missionController) return "Error: MissionController not available";

    QGeoCoordinate coordinate;
    if (args.contains("coordinates") && !args["coordinates"].toString().isEmpty()) {
        QString coordStr = args["coordinates"].toString();
        QStringList parts = coordStr.split(',');
        if (parts.size() == 2) {
            coordinate = QGeoCoordinate(parts[0].trimmed().toDouble(),
                                        parts[1].trimmed().toDouble());
        }
    }
    if (!coordinate.isValid()) {
        auto* vehicle = activeVehicle();
        if (vehicle) coordinate = vehicle->homePosition();
    }

    // insertLandItem() handles vehicle-type routing internally:
    //   fixed-wing → FixedWingLandingComplexItem
    //   VTOL       → VTOLLandingComplexItem
    //   multirotor → MAV_CMD_NAV_RETURN_TO_LAUNCH (RTL)
    VisualMissionItem* item = missionController->insertLandItem(coordinate, -1);
    if (!item) return "Error: Failed to insert land item";

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem && args.contains("altitude")) {
        simpleItem->altitude()->setRawValue(args["altitude"].toDouble());
    }

    int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    return QStringLiteral("Land command added to mission (Item %1)").arg(seqNum);
}
```

**Verification:** Both tools compile. `name()` returns correct strings.

---

### Task 7.3: Create AddLoiterTool and AddRTLTool

**Objective:** Implement loiter (orbit) and RTL (return-to-launch) tools. Neither has a dedicated QGC insert method — they use `insertSimpleMissionItem()` then change the command.

**Files:**
- Create: `src/AIAgent/Tools/AddLoiterTool.h`
- Create: `src/AIAgent/Tools/AddLoiterTool.cpp`
- Create: `src/AIAgent/Tools/AddRTLTool.h`
- Create: `src/AIAgent/Tools/AddRTLTool.cpp`

**AddLoiterTool.h:**

```cpp
#pragma once

#include "../AgentToolBase.h"

class AddLoiterTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit AddLoiterTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_loiter"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        // Ground vehicles cannot loiter/orbit
        return vehicleType != "ground";
    }
};
```

**AddLoiterTool.cpp key logic:**

```cpp
QString AddLoiterTool::description() const {
    return QStringLiteral(
        "Add circular orbit/loiter pattern at specified location. Use when user wants "
        "drone to fly in circles, orbit, or loiter. Use for commands like 'orbit', 'circle', "
        "'loiter', or when radius is mentioned. Specify coordinates OR distance/heading/reference. "
        "Do not mix location systems."
    );
}

QJsonObject AddLoiterTool::parameters() const {
    QJsonObject schema;
    schema["type"] = "object";
    QJsonObject props;
    props["coordinates"] = QJsonObject{
        {"type", "string"},
        {"description", "GPS coordinates as 'lat,lon' for orbit center."}
    };
    props["altitude"] = QJsonObject{
        {"type", "number"},
        {"description", "Altitude for the orbit pattern in meters. Default = 50 meters."}
    };
    props["altitude_units"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"meters", "feet"}}
    };
    props["radius"] = QJsonObject{
        {"type", "number"},
        {"description", "Radius of the circular orbit in meters. Default = 50 meters."}
    };
    props["radius_units"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"meters", "feet"}}
    };
    props["seq"] = QJsonObject{
        {"type", "integer"},
        {"description", "Position to insert in mission (1-based). Omit to add at end."}
    };
    props["distance"] = QJsonObject{
        {"type", "number"},
        {"description", "Distance to orbit center from reference point in meters."}
    };
    props["heading"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"north", "northeast", "east", "southeast", "south",
                            "southwest", "west", "northwest"}}
    };
    props["relative_reference_frame"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"origin", "last_waypoint"}}
    };
    schema["properties"] = props;
    return schema;
}

QString AddLoiterTool::execute(const QJsonObject& args) {
    auto* controller = planController();
    if (!controller) return "Error: PlanMasterController not available";
    auto* missionController = controller->missionController();
    if (!missionController) return "Error: MissionController not available";

    // Parse coordinates (same pattern as AddWaypointTool)
    QGeoCoordinate coordinate;
    if (args.contains("coordinates") && !args["coordinates"].toString().isEmpty()) {
        QString coordStr = args["coordinates"].toString();
        QStringList parts = coordStr.split(',');
        if (parts.size() == 2) {
            coordinate = QGeoCoordinate(parts[0].trimmed().toDouble(),
                                        parts[1].trimmed().toDouble());
        }
    }
    if (!coordinate.isValid()) {
        auto* vehicle = activeVehicle();
        if (vehicle) coordinate = vehicle->homePosition();
    }
    if (!coordinate.isValid()) return "Error: No coordinates provided";

    int insertIndex = args.contains("seq") ? args["seq"].toInt() : -1;

    // Insert as simple item, then change command to LOITER_TURNS
    VisualMissionItem* item = missionController->insertSimpleMissionItem(coordinate, insertIndex);
    if (!item) return "Error: Failed to insert loiter item";

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem) {
        // Change command from WAYPOINT to LOITER_TURNS
        simpleItem->setCommand(MAV_CMD_NAV_LOITER_TURNS);

        // Set altitude
        if (args.contains("altitude")) {
            double alt = args["altitude"].toDouble();
            if (args.contains("altitude_units") && args["altitude_units"].toString() == "feet") {
                alt *= 0.3048;
            }
            simpleItem->altitude()->setRawValue(alt);
        }

        // Set radius
        if (args.contains("radius")) {
            double radius = args["radius"].toDouble();
            if (args.contains("radius_units") && args["radius_units"].toString() == "feet") {
                radius *= 0.3048;
            }
            simpleItem->setRadius(radius);
        }
    }

    int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    QString altMsg = args.contains("altitude")
        ? QString("%1 %2").arg(args["altitude"].toDouble()).arg(args["altitude_units"].toString("meters"))
        : "default";
    QString radMsg = args.contains("radius")
        ? QString("%1 %2").arg(args["radius"].toDouble()).arg(args["radius_units"].toString("meters"))
        : "default";

    return QStringLiteral("Loiter command added to mission: (%1, %2), Alt=%3, Radius=%4 (Item %5)")
        .arg(coordinate.latitude(), 0, 'f', 6)
        .arg(coordinate.longitude(), 0, 'f', 6)
        .arg(altMsg).arg(radMsg).arg(seqNum);
}
```

**AddRTLTool.h:**

```cpp
#pragma once

#include "../AgentToolBase.h"

class AddRTLTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit AddRTLTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_rtl"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        Q_UNUSED(vehicleType);
        return true;  // All vehicle types support RTL
    }
};
```

**AddRTLTool.cpp key logic:**

```cpp
QString AddRTLTool::description() const {
    return QStringLiteral(
        "Add return to launch command to automatically fly back to takeoff point and land. "
        "Always inserted as the LAST mission item. Use when the drone should return home, "
        "land, or come back."
    );
}

QJsonObject AddRTLTool::parameters() const {
    QJsonObject schema;
    schema["type"] = "object";
    QJsonObject props;
    props["altitude"] = QJsonObject{
        {"type", "number"},
        {"description", "Return altitude in meters. Specify only if user mentions specific "
                        "return height. Default = 50 meters."}
    };
    props["altitude_units"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"meters", "feet"}}
    };
    schema["properties"] = props;
    return schema;
}

QString AddRTLTool::execute(const QJsonObject& args) {
    auto* controller = planController();
    if (!controller) return "Error: PlanMasterController not available";
    auto* missionController = controller->missionController();
    if (!missionController) return "Error: MissionController not available";

    // RTL uses home position as coordinate, always appended at end (-1)
    QGeoCoordinate coordinate;
    auto* vehicle = activeVehicle();
    if (vehicle) coordinate = vehicle->homePosition();

    // Insert simple item, then change command to RETURN_TO_LAUNCH
    VisualMissionItem* item = missionController->insertSimpleMissionItem(coordinate, -1);
    if (!item) return "Error: Failed to insert RTL item";

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem) {
        simpleItem->setCommand(MAV_CMD_NAV_RETURN_TO_LAUNCH);
        if (args.contains("altitude")) {
            double alt = args["altitude"].toDouble();
            if (args.contains("altitude_units") && args["altitude_units"].toString() == "feet") {
                alt *= 0.3048;
            }
            simpleItem->altitude()->setRawValue(alt);
        }
    }

    int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    return QStringLiteral("Return to Launch command added to mission (Item %1)").arg(seqNum);
}
```

**Verification:** Both tools compile. Loiter tool sets `MAV_CMD_NAV_LOITER_TURNS`. RTL tool sets `MAV_CMD_NAV_RETURN_TO_LAUNCH`. Loiter excluded for ground vehicles.

---

### Task 7.4: Create AddSurveyTool

**Objective:** Implement survey tool using QGC's complex mission item (Survey pattern).

**Files:**
- Create: `src/AIAgent/Tools/AddSurveyTool.h`
- Create: `src/AIAgent/Tools/AddSurveyTool.cpp`

**AddSurveyTool.h:**

```cpp
#pragma once

#include "../AgentToolBase.h"

class AddSurveyTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit AddSurveyTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_survey"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        Q_UNUSED(vehicleType);
        return true;
    }
};
```

**AddSurveyTool.cpp key logic:**

```cpp
QString AddSurveyTool::description() const {
    return QStringLiteral(
        "Create survey pattern for area coverage. Uses center point and the map center "
        "to generate a survey grid pattern. Use for survey commands like 'survey this area', "
        "'map the region', or 'scan the zone'. Specify coordinates OR distance/heading/reference. "
        "Do not mix location systems."
    );
}

QJsonObject AddSurveyTool::parameters() const {
    QJsonObject schema;
    schema["type"] = "object";
    QJsonObject props;
    props["coordinates"] = QJsonObject{
        {"type", "string"},
        {"description", "GPS coordinates as 'lat,lon' for survey center."}
    };
    props["altitude"] = QJsonObject{
        {"type", "number"},
        {"description", "Flight altitude for the survey in meters. Default = 50 meters."}
    };
    props["altitude_units"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"meters", "feet"}}
    };
    props["seq"] = QJsonObject{
        {"type", "integer"},
        {"description", "Position to insert survey in mission (1-based). Omit to add at end."}
    };
    props["distance"] = QJsonObject{
        {"type", "number"},
        {"description", "Distance to survey center from reference point in meters."}
    };
    props["heading"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"north", "northeast", "east", "southeast", "south",
                            "southwest", "west", "northwest"}}
    };
    props["relative_reference_frame"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"origin", "last_waypoint"}}
    };
    schema["properties"] = props;
    return schema;
}

QString AddSurveyTool::execute(const QJsonObject& args) {
    auto* controller = planController();
    if (!controller) return "Error: PlanMasterController not available";
    auto* missionController = controller->missionController();
    if (!missionController) return "Error: MissionController not available";

    QGeoCoordinate coordinate;
    if (args.contains("coordinates") && !args["coordinates"].toString().isEmpty()) {
        QString coordStr = args["coordinates"].toString();
        QStringList parts = coordStr.split(',');
        if (parts.size() == 2) {
            coordinate = QGeoCoordinate(parts[0].trimmed().toDouble(),
                                        parts[1].trimmed().toDouble());
        }
    }
    if (!coordinate.isValid()) {
        auto* vehicle = activeVehicle();
        if (vehicle) coordinate = vehicle->homePosition();
    }
    if (!coordinate.isValid()) return "Error: No coordinates provided";

    int insertIndex = args.contains("seq") ? args["seq"].toInt() : -1;

    // Insert Survey as a complex mission item
    VisualMissionItem* item = missionController->insertComplexMissionItem(
        "Survey", coordinate, insertIndex);
    if (!item) return "Error: Failed to insert survey item";

    // Survey complex item altitude can be set via the item's altitude fact
    // (ComplexMissionItem / Survey complex item has its own altitude property)
    if (args.contains("altitude")) {
        auto* complexItem = qobject_cast<ComplexMissionItem*>(item);
        if (complexItem) {
            // Survey altitude is set through the TransectStyle complex item's altitude fact
            // This will be refined once we can test against actual Survey item properties
        }
    }

    return QStringLiteral("Survey pattern created at (%1, %2) (Item inserted)")
        .arg(coordinate.latitude(), 0, 'f', 6)
        .arg(coordinate.longitude(), 0, 'f', 6);
}
```

**Verification:** Compiles. `insertComplexMissionItem("Survey", ...)` is the correct QGC API.

---

### Task 7.5: Create AddTransitionTool

**Objective:** Implement VTOL transition tool — simplest tool, VTOL-only.

**Files:**
- Create: `src/AIAgent/Tools/AddTransitionTool.h`
- Create: `src/AIAgent/Tools/AddTransitionTool.cpp`

**AddTransitionTool.h:**

```cpp
#pragma once

#include "../AgentToolBase.h"

class AddTransitionTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit AddTransitionTool(QObject* parent = nullptr) : AgentToolBase(parent) {}

    QString name() const override { return QStringLiteral("add_transition"); }
    QString description() const override;
    QJsonObject parameters() const override;
    QString execute(const QJsonObject& args) override;

    bool availableInMode(const QString& mode) const override {
        Q_UNUSED(mode);
        return true;
    }
    bool availableForVehicle(const QString& vehicleType) const override {
        // VTOL transition is ONLY for VTOL vehicles
        return vehicleType == "vtol";
    }
};
```

**AddTransitionTool.cpp key logic:**

```cpp
QString AddTransitionTool::description() const {
    return QStringLiteral(
        "Add VTOL transition command (multirotor to fixed-wing or vice versa). "
        "Use when user wants to switch flight mode for a VTOL drone. "
        "'transition to fixed wing' or 'switch to multirotor mode'."
    );
}

QJsonObject AddTransitionTool::parameters() const {
    QJsonObject schema;
    schema["type"] = "object";
    QJsonObject props;
    props["target_state"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{"vtol_mc_to_fw", "vtol_fw_to_mc"}},
        {"description", "Transition direction: 'vtol_mc_to_fw' (multirotor→fixed-wing) "
                        "or 'vtol_fw_to_mc' (fixed-wing→multirotor)."}
    };
    props["seq"] = QJsonObject{
        {"type", "integer"},
        {"description", "Position to insert in mission (1-based). Omit to add at end."}
    };
    schema["properties"] = props;

    QJsonArray required;
    required.append("target_state");
    schema["required"] = required;

    return schema;
}

QString AddTransitionTool::execute(const QJsonObject& args) {
    auto* controller = planController();
    if (!controller) return "Error: PlanMasterController not available";
    auto* missionController = controller->missionController();
    if (!missionController) return "Error: MissionController not available";

    if (!args.contains("target_state")) {
        return "Error: target_state is required. Use 'vtol_mc_to_fw' or 'vtol_fw_to_mc'.";
    }

    QString targetState = args["target_state"].toString();
    if (targetState != "vtol_mc_to_fw" && targetState != "vtol_fw_to_mc") {
        return "Error: Invalid target_state. Use 'vtol_mc_to_fw' or 'vtol_fw_to_mc'.";
    }

    int insertIndex = args.contains("seq") ? args["seq"].toInt() : -1;

    // Transition items use coordinate (0,0) — they're command items, not nav items
    QGeoCoordinate coordinate(0, 0);
    VisualMissionItem* item = missionController->insertSimpleMissionItem(coordinate, insertIndex);
    if (!item) return "Error: Failed to insert transition item";

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem) {
        simpleItem->setCommand(MAV_CMD_DO_VTOL_TRANSITION);
        // param1: MAV_VTOL_STATE — 3=MC→FW, 4=FW→MC
        int transitionState = (targetState == "vtol_mc_to_fw") ? 3 : 4;
        simpleItem->missionItem().setParam1(transitionState);
    }

    int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    return QStringLiteral("VTOL transition (%1) added to mission (Item %2)")
        .arg(targetState).arg(seqNum);
}
```

**Verification:** Compiles. Only available for VTOL vehicles. `target_state` is required.

---

### Task 7.6: Create RegisterAllTools and update CMakeLists

**Objective:** Add a registration function that instantiates all 7 tools and registers them with the AgentToolRegistry. Update CMake to compile all tool sources.

**Files:**
- Create: `src/AIAgent/Tools/RegisterTools.h`
- Create: `src/AIAgent/Tools/RegisterTools.cpp`
- Modify: `src/AIAgent/Tools/CMakeLists.txt` (add all sources)
- Modify: `src/AIAgent/CMakeLists.txt` (add Tools subdirectory)

**RegisterTools.h:**

```cpp
#pragma once

class AgentToolRegistry;

/// Instantiate and register all mission add tools with the given registry.
/// Call this once during application startup (e.g., from AgentController constructor).
void registerAllTools(AgentToolRegistry* registry);
```

**RegisterTools.cpp:**

```cpp
#include "RegisterTools.h"
#include "../AgentToolRegistry.h"
#include "AddWaypointTool.h"
#include "AddTakeoffTool.h"
#include "AddLandTool.h"
#include "AddLoiterTool.h"
#include "AddRTLTool.h"
#include "AddSurveyTool.h"
#include "AddTransitionTool.h"

void registerAllTools(AgentToolRegistry* registry) {
    if (!registry) return;

    registry->registerTool(new AddWaypointTool(registry));
    registry->registerTool(new AddTakeoffTool(registry));
    registry->registerTool(new AddLandTool(registry));
    registry->registerTool(new AddLoiterTool(registry));
    registry->registerTool(new AddRTLTool(registry));
    registry->registerTool(new AddSurveyTool(registry));
    registry->registerTool(new AddTransitionTool(registry));
}
```

**Updated Tools/CMakeLists.txt:**

```cmake
target_sources(AIAgentModule
    PRIVATE
        AddWaypointTool.h
        AddWaypointTool.cpp
        AddTakeoffTool.h
        AddTakeoffTool.cpp
        AddLandTool.h
        AddLandTool.cpp
        AddLoiterTool.h
        AddLoiterTool.cpp
        AddRTLTool.h
        AddRTLTool.cpp
        AddSurveyTool.h
        AddSurveyTool.cpp
        AddTransitionTool.h
        AddTransitionTool.cpp
        RegisterTools.h
        RegisterTools.cpp
)

target_include_directories(AIAgentModule
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
```

**Updated src/AIAgent/CMakeLists.txt:** Add `add_subdirectory(Tools)` after the existing `target_sources` block, before `target_include_directories`.

**Verification:** All 7 tools + RegisterTools compile. `registerAllTools()` creates and registers all tools.

---

### Task 7.7: Write CTest for all 7 tools

**Objective:** Unit tests verifying tool names, JSON schema validity, and mode/vehicle availability.

**Files:**
- Create: `test/AIAgent/AddToolsTest.h`
- Create: `test/AIAgent/AddToolsTest.cc`
- Modify: `test/AIAgent/CMakeLists.txt` (add test sources)
- Modify: `test/CMakeLists.txt` (register test, if needed)

**AddToolsTest.h:**

```cpp
#pragma once

#include <QtTest/QTest>

class AddToolsTest : public QObject {
    Q_OBJECT
private slots:
    void _nameTest();                    // All 7 tools return correct name()
    void _schemaValidTest();             // Each tool's parameters() is a valid JSON Schema
    void _schemaHasTypeObjectTest();     // Each schema has "type": "object"
    void _waypointSchemaTest();          // add_waypoint has coordinates, altitude, seq
    void _takeoffSchemaTest();           // add_takeoff has altitude, heading
    void _loiterSchemaTest();            // add_loiter has coordinates, altitude, radius
    void _rtlSchemaTest();               // add_rtl has altitude only
    void _surveySchemaTest();            // add_survey has coordinates, altitude
    void _landSchemaTest();              // add_land has coordinates, altitude
    void _transitionSchemaTest();        // add_transition has target_state (required)
    void _availabilityTest();            // Mode and vehicle availability for all tools
    void _takeoffVehicleFilterTest();    // Takeoff only for fixed_wing/vtol
    void _loiterVehicleFilterTest();     // Loiter excluded for ground
    void _transitionVehicleFilterTest(); // Transition only for vtol
    void _registerAllToolsTest();        // registerAllTools adds 7 tools to registry
};
```

**AddToolsTest.cc key test implementations:**

```cpp
#include "AddToolsTest.h"
#include "src/AIAgent/Tools/RegisterTools.h"
#include "src/AIAgent/Tools/AddWaypointTool.h"
#include "src/AIAgent/Tools/AddTakeoffTool.h"
// ... etc for all tools

void AddToolsTest::_nameTest() {
    QCOMPARE(AddWaypointTool().name(), "add_waypoint");
    QCOMPARE(AddTakeoffTool().name(), "add_takeoff");
    QCOMPARE(AddLandTool().name(), "add_land");
    QCOMPARE(AddLoiterTool().name(), "add_loiter");
    QCOMPARE(AddRTLTool().name(), "add_rtl");
    QCOMPARE(AddSurveyTool().name(), "add_survey");
    QCOMPARE(AddTransitionTool().name(), "add_transition");
}

void AddToolsTest::_schemaValidTest() {
    // Each tool's parameters() returns valid JSON (QJsonObject is always valid,
    // but check it has "type" and "properties")
    AddWaypointTool wp;
    QJsonObject schema = wp.parameters();
    QVERIFY(schema.contains("type"));
    QVERIFY(schema.contains("properties"));
    // Repeat for all 7 tools...
}

void AddToolsTest::_availabilityTest() {
    AddWaypointTool wp;
    QVERIFY(wp.availableInMode("mission"));
    QVERIFY(wp.availableInMode("command"));
    QVERIFY(wp.availableForVehicle("fixed_wing"));
    QVERIFY(wp.availableForVehicle("multi_rotor"));
    QVERIFY(wp.availableForVehicle("vtol"));
    QVERIFY(wp.availableForVehicle("ground"));

    AddTakeoffTool to;
    QVERIFY(!to.availableForVehicle("multi_rotor"));
    QVERIFY(!to.availableForVehicle("ground"));
    QVERIFY(to.availableForVehicle("fixed_wing"));
    QVERIFY(to.availableForVehicle("vtol"));

    AddLoiterTool li;
    QVERIFY(!li.availableForVehicle("ground"));
    QVERIFY(li.availableForVehicle("multi_rotor"));
    QVERIFY(li.availableForVehicle("fixed_wing"));

    AddTransitionTool tr;
    QVERIFY(!tr.availableForVehicle("multi_rotor"));
    QVERIFY(!tr.availableForVehicle("fixed_wing"));
    QVERIFY(!tr.availableForVehicle("ground"));
    QVERIFY(tr.availableForVehicle("vtol"));
}

void AddToolsTest::_registerAllToolsTest() {
    AgentToolRegistry registry;
    registerAllTools(&registry);
    // Verify all 7 tools registered
    QVERIFY(registry.getToolByName("add_waypoint") != nullptr);
    QVERIFY(registry.getToolByName("add_takeoff") != nullptr);
    QVERIFY(registry.getToolByName("add_land") != nullptr);
    QVERIFY(registry.getToolByName("add_loiter") != nullptr);
    QVERIFY(registry.getToolByName("add_rtl") != nullptr);
    QVERIFY(registry.getToolByName("add_survey") != nullptr);
    QVERIFY(registry.getToolByName("add_transition") != nullptr);
}
```

**Verification:** `AddToolsTest` passes all 15 test cases in Docker.

---

### Task 7.8: Docker build verification

**Objective:** Full Docker rebuild to confirm all 7 tools + tests compile and link.

**Steps:**
1. Reconfigure CMake: `qt-cmake -S /project/source -B /project/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DQGC_BUILD_TESTING=ON`
2. Build: `ninja -C /project/build QGroundControlModule`
3. Build test: `ninja -C /project/build AddToolsTest` (if registered as CMake target)
4. Verify: no compile/link errors, all tool sources listed in AIAgentModule

**Verification:** Clean build. All compile targets succeed.

---

### Task 7.9: Git commit and push

**Objective:** Commit all PR 7 changes with clear messages.

**Files to commit:**
- `src/AIAgent/Tools/AddWaypointTool.h/.cpp`
- `src/AIAgent/Tools/AddTakeoffTool.h/.cpp`
- `src/AIAgent/Tools/AddLandTool.h/.cpp`
- `src/AIAgent/Tools/AddLoiterTool.h/.cpp`
- `src/AIAgent/Tools/AddRTLTool.h/.cpp`
- `src/AIAgent/Tools/AddSurveyTool.h/.cpp`
- `src/AIAgent/Tools/AddTransitionTool.h/.cpp`
- `src/AIAgent/Tools/RegisterTools.h/.cpp`
- `src/AIAgent/Tools/CMakeLists.txt`
- `src/AIAgent/CMakeLists.txt` (modified — added Tools subdirectory)
- `test/AIAgent/AddToolsTest.h/.cc` (new test)
- `test/AIAgent/CMakeLists.txt` (modified — added test sources)
- `docs/plans/2026-04-25-pr7-mission-add-tools.md` (this plan)

**Commit:**
```bash
git add src/AIAgent/Tools/ src/AIAgent/CMakeLists.txt test/AIAgent/ docs/plans/
git commit -m "feat: implement 7 mission add tools ported from mav-agent

- AddWaypointTool: insertSimpleMissionItem with coordinates & altitude
- AddTakeoffTool: insertTakeoffItem (fixed_wing/vtol only)
- AddLandTool: insertLandItem (all vehicles, auto-routes by type)
- AddLoiterTool: insertSimple + MAV_CMD_NAV_LOITER_TURNS (not ground)
- AddRTLTool: insertSimple + MAV_CMD_NAV_RETURN_TO_LAUNCH
- AddSurveyTool: insertComplexMissionItem('Survey')
- AddTransitionTool: insertSimple + MAV_CMD_DO_VTOL_TRANSITION (vtol only)
- RegisterAllTools function for bulk registration
- CTest: name, schema, availability filtering for all 7 tools"
```

**Push:**
```bash
git push -u origin feat/ai-agent-pr7-mission-add-tools
```

**Verification:** `git log --oneline -1` shows the commit. Branch pushed to remote.

---

## Dependencies & Risks

- **Task 7.1 → 7.2-7.5:** AddWaypointTool is the reference implementation; remaining tools follow the same pattern.
- **Task 7.6 depends on 7.1-7.5:** RegisterAllTools needs all tool headers.
- **Task 7.7 depends on 7.6:** Tests register tools and verify schemas.
- **Task 7.8 depends on 7.7:** Docker build includes test compilation.
- **Task 7.9 depends on 7.8:** Commit after successful build.

**Risk: MissionController access** — `planController()` and `activeVehicle()` are currently stubs returning nullptr. Tool `execute()` methods will return error strings until these are wired up in PR 10 (AgentController). This is fine — schemas and availability filtering are testable now, and execute will work once the parent controller injection happens.

**Risk: Loiter RTL via setCommand** — Changing command via `setCommand()` after `insertSimpleMissionItem()` may trigger recalculations. Testing in Docker with the actual QGC runtime will confirm this works. If it causes issues, we can use `_insertSimpleMissionItemWorker()` directly (it's private but we could add a public wrapper).

**Risk: Survey altitude** — Setting altitude on a Survey complex item may require accessing specific properties of the `SurveyComplexItem`. The initial implementation handles this with a TODO; exact property access will be refined during Docker runtime testing.

## Not In Scope

- `execute()` runtime testing with real QGC (requires PR 10 AgentController wiring)
- Relative coordinate resolution (distance/heading/reference_frame) — deferred to PR 9
- MGRS coordinate support — mav-agent has it but v1 omits it
- `search_target` / `detection_behavior` fields — AI vision features, future PR
- Human-in-the-loop confirmation via AIAgentActionCard — PR 12
- Edit tools (update, delete, move, reorder) — PR 9
- Guided action tools — PR 9