// Standalone test for mission edit + guided action tools — schema and availability checks only.
// Uses stub tool classes (no execute() deps needed) so it can run headlessly
// in Docker without the full QGC codebase.
// Run in Docker: ./test_edit_guided_tools_schema

#include "AgentToolBase.h"
#include "AgentToolRegistry.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <iostream>
#include <memory>

// ============================================================================
// Mission Edit Tool Stubs (mission mode only)
// ============================================================================

class StubUpdateMissionItemTool : public AgentToolBase {
public:
    explicit StubUpdateMissionItemTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("update_mission_item"); }
    QString description() const override { return QStringLiteral("Update properties of an existing mission item."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["seq"] = QJsonObject{{"type", "integer"}, {"description", "1-based sequence number of item to update"}};
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "New altitude in meters"}};
        props["altitude_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters", "feet"}}};
        props["radius"] = QJsonObject{{"type", "number"}, {"description", "New loiter/orbit radius in meters"}};
        props["radius_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters", "feet"}}};
        props["speed"] = QJsonObject{{"type", "number"}, {"description", "New speed in m/s"}};
        schema["properties"] = props;
        QJsonArray required;
        required.append(QStringLiteral("seq"));
        schema["required"] = required;
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("mission"); }
    bool availableForVehicle(const QString&) const override { return true; }
};

class StubDeleteMissionItemTool : public AgentToolBase {
public:
    explicit StubDeleteMissionItemTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("delete_mission_item"); }
    QString description() const override { return QStringLiteral("Delete a mission item from the plan."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["seq"] = QJsonObject{{"type", "integer"}, {"description", "1-based sequence number of item to delete"}};
        schema["properties"] = props;
        QJsonArray required;
        required.append(QStringLiteral("seq"));
        schema["required"] = required;
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("mission"); }
    bool availableForVehicle(const QString&) const override { return true; }
};

class StubMoveItemTool : public AgentToolBase {
public:
    explicit StubMoveItemTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("move_item"); }
    QString description() const override { return QStringLiteral("Move a mission item to new GPS coordinates."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["seq"] = QJsonObject{{"type", "integer"}, {"description", "1-based sequence number of item to move"}};
        props["coordinates"] = QJsonObject{{"type", "string"}, {"description", "New GPS coordinates as lat,lon"}};
        schema["properties"] = props;
        QJsonArray required;
        required.append(QStringLiteral("seq"));
        required.append(QStringLiteral("coordinates"));
        schema["required"] = required;
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("mission"); }
    bool availableForVehicle(const QString&) const override { return true; }
};

class StubReorderItemTool : public AgentToolBase {
public:
    explicit StubReorderItemTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("reorder_item"); }
    QString description() const override { return QStringLiteral("Change the order of mission items."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["from_seq"] = QJsonObject{{"type", "integer"}, {"description", "Current 1-based sequence number"}};
        props["to_seq"] = QJsonObject{{"type", "integer"}, {"description", "Target 1-based position"}};
        schema["properties"] = props;
        QJsonArray required;
        required.append(QStringLiteral("from_seq"));
        required.append(QStringLiteral("to_seq"));
        schema["required"] = required;
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("mission"); }
    bool availableForVehicle(const QString&) const override { return true; }
};

// ============================================================================
// Guided Action Tool Stubs (command mode only)
// ============================================================================

class StubGuidedTakeoffTool : public AgentToolBase {
public:
    explicit StubGuidedTakeoffTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("guided_takeoff"); }
    QString description() const override { return QStringLiteral("Command the vehicle to take off."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "Takeoff altitude in meters above launch"}};
        props["altitude_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters", "feet"}}};
        schema["properties"] = props;
        QJsonArray required;
        required.append(QStringLiteral("altitude"));
        schema["required"] = required;
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("command"); }
    bool availableForVehicle(const QString& vt) const override {
        return vt == QStringLiteral("fixed_wing") || vt == QStringLiteral("vtol");
    }
};

class StubGuidedGoToTool : public AgentToolBase {
public:
    explicit StubGuidedGoToTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("guided_goto"); }
    QString description() const override { return QStringLiteral("Command the vehicle to fly to GPS coordinates."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["coordinates"] = QJsonObject{{"type", "string"}, {"description", "GPS coordinates as lat,lon"}};
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "Altitude to fly at in meters"}};
        schema["properties"] = props;
        QJsonArray required;
        required.append(QStringLiteral("coordinates"));
        schema["required"] = required;
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("command"); }
    bool availableForVehicle(const QString& vt) const override {
        return vt != QStringLiteral("ground") && vt != QStringLiteral("rover");
    }
};

class StubGuidedRTLTool : public AgentToolBase {
public:
    explicit StubGuidedRTLTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("guided_rtl"); }
    QString description() const override { return QStringLiteral("Command the vehicle to return to launch."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["smart_rtl"] = QJsonObject{{"type", "boolean"}, {"description", "Use smart RTL if available"}};
        schema["properties"] = props;
        schema["required"] = QJsonArray();
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("command"); }
    bool availableForVehicle(const QString&) const override { return true; }
};

class StubGuidedLandTool : public AgentToolBase {
public:
    explicit StubGuidedLandTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("guided_land"); }
    QString description() const override { return QStringLiteral("Command the vehicle to land at current position."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        schema["properties"] = QJsonObject();
        schema["required"] = QJsonArray();
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("command"); }
    bool availableForVehicle(const QString&) const override { return true; }
};

class StubGuidedOrbitTool : public AgentToolBase {
public:
    explicit StubGuidedOrbitTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("guided_orbit"); }
    QString description() const override { return QStringLiteral("Command the vehicle to orbit around a point."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["coordinates"] = QJsonObject{{"type", "string"}, {"description", "Center point GPS coordinates as lat,lon"}};
        props["radius"] = QJsonObject{{"type", "number"}, {"description", "Orbit radius in meters"}};
        props["radius_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters", "feet"}}};
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "Orbit altitude AMSL in meters"}};
        props["altitude_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters", "feet"}}};
        schema["properties"] = props;
        QJsonArray required;
        required.append(QStringLiteral("coordinates"));
        required.append(QStringLiteral("radius"));
        required.append(QStringLiteral("altitude"));
        schema["required"] = required;
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("command"); }
    bool availableForVehicle(const QString& vt) const override {
        return vt == QStringLiteral("rotor") || vt == QStringLiteral("vtol") || vt == QStringLiteral("multi_rotor");
    }
};

// ============================================================================
// Test Harness
// ============================================================================

static int passed = 0;
static int failed = 0;

#define CHECK(expr, msg) \
    do { \
        if (expr) { \
            std::cout << "PASS: " << msg << std::endl; \
            passed++; \
        } else { \
            std::cout << "FAIL: " << msg << std::endl; \
            failed++; \
        } \
    } while(0)

static QJsonObject toolProperties(AgentToolBase& tool) {
    return tool.parameters().value("properties").toObject();
}
static QJsonArray toolRequired(AgentToolBase& tool) {
    return tool.parameters().value("required").toArray();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // Create edit tool instances
    StubUpdateMissionItemTool updateItem;
    StubDeleteMissionItemTool deleteItem;
    StubMoveItemTool moveItem;
    StubReorderItemTool reorderItem;

    // Create guided tool instances
    StubGuidedTakeoffTool guidedTakeoff;
    StubGuidedGoToTool guidedGoTo;
    StubGuidedRTLTool guidedRTL;
    StubGuidedLandTool guidedLand;
    StubGuidedOrbitTool guidedOrbit;

    // ========================================================================
    // Tool names
    // ========================================================================
    CHECK(updateItem.name() == "update_mission_item", "name: update_mission_item");
    CHECK(deleteItem.name() == "delete_mission_item", "name: delete_mission_item");
    CHECK(moveItem.name() == "move_item", "name: move_item");
    CHECK(reorderItem.name() == "reorder_item", "name: reorder_item");
    CHECK(guidedTakeoff.name() == "guided_takeoff", "name: guided_takeoff");
    CHECK(guidedGoTo.name() == "guided_goto", "name: guided_goto");
    CHECK(guidedRTL.name() == "guided_rtl", "name: guided_rtl");
    CHECK(guidedLand.name() == "guided_land", "name: guided_land");
    CHECK(guidedOrbit.name() == "guided_orbit", "name: guided_orbit");

    // ========================================================================
    // Schema checks — Edit tools
    // ========================================================================
    CHECK(toolProperties(updateItem).contains("seq"), "update has seq");
    CHECK(toolProperties(updateItem).contains("altitude"), "update has altitude");
    CHECK(toolProperties(updateItem).contains("radius"), "update has radius");
    CHECK(toolProperties(updateItem).contains("speed"), "update has speed");
    CHECK(toolRequired(updateItem).size() == 1, "update: 1 required (seq)");

    CHECK(toolProperties(deleteItem).contains("seq"), "delete has seq");
    CHECK(toolRequired(deleteItem).size() == 1, "delete: 1 required (seq)");

    CHECK(toolProperties(moveItem).contains("seq"), "move has seq");
    CHECK(toolProperties(moveItem).contains("coordinates"), "move has coordinates");
    CHECK(toolRequired(moveItem).size() == 2, "move: 2 required (seq, coordinates)");

    CHECK(toolProperties(reorderItem).contains("from_seq"), "reorder has from_seq");
    CHECK(toolProperties(reorderItem).contains("to_seq"), "reorder has to_seq");
    CHECK(toolRequired(reorderItem).size() == 2, "reorder: 2 required (from_seq, to_seq)");

    // ========================================================================
    // Schema checks — Guided tools
    // ========================================================================
    CHECK(toolProperties(guidedTakeoff).contains("altitude"), "guided_takeoff has altitude");
    CHECK(toolRequired(guidedTakeoff).size() == 1, "guided_takeoff: 1 required (altitude)");

    CHECK(toolProperties(guidedGoTo).contains("coordinates"), "guided_goto has coordinates");
    CHECK(toolRequired(guidedGoTo).size() == 1, "guided_goto: 1 required (coordinates)");

    CHECK(toolProperties(guidedRTL).contains("smart_rtl"), "guided_rtl has smart_rtl");
    CHECK(toolRequired(guidedRTL).size() == 0, "guided_rtl: 0 required");

    CHECK(toolProperties(guidedLand).size() == 0, "guided_land: 0 properties");
    CHECK(toolRequired(guidedLand).size() == 0, "guided_land: 0 required");

    CHECK(toolProperties(guidedOrbit).contains("coordinates"), "guided_orbit has coordinates");
    CHECK(toolProperties(guidedOrbit).contains("radius"), "guided_orbit has radius");
    CHECK(toolProperties(guidedOrbit).contains("altitude"), "guided_orbit has altitude");
    CHECK(toolRequired(guidedOrbit).size() == 3, "guided_orbit: 3 required");

    // ========================================================================
    // Mode availability — Edit tools (mission ONLY)
    // ========================================================================
    CHECK(updateItem.availableInMode("mission"), "update in mission");
    CHECK(!updateItem.availableInMode("command"), "update NOT in command");
    CHECK(deleteItem.availableInMode("mission"), "delete in mission");
    CHECK(!deleteItem.availableInMode("command"), "delete NOT in command");
    CHECK(moveItem.availableInMode("mission"), "move in mission");
    CHECK(!moveItem.availableInMode("command"), "move NOT in command");
    CHECK(reorderItem.availableInMode("mission"), "reorder in mission");
    CHECK(!reorderItem.availableInMode("command"), "reorder NOT in command");

    // ========================================================================
    // Mode availability — Guided tools (command ONLY)
    // ========================================================================
    CHECK(!guidedTakeoff.availableInMode("mission"), "guided_takeoff NOT in mission");
    CHECK(guidedTakeoff.availableInMode("command"), "guided_takeoff in command");
    CHECK(!guidedGoTo.availableInMode("mission"), "guided_goto NOT in mission");
    CHECK(guidedGoTo.availableInMode("command"), "guided_goto in command");
    CHECK(!guidedRTL.availableInMode("mission"), "guided_rtl NOT in mission");
    CHECK(guidedRTL.availableInMode("command"), "guided_rtl in command");
    CHECK(!guidedLand.availableInMode("mission"), "guided_land NOT in mission");
    CHECK(guidedLand.availableInMode("command"), "guided_land in command");
    CHECK(!guidedOrbit.availableInMode("mission"), "guided_orbit NOT in mission");
    CHECK(guidedOrbit.availableInMode("command"), "guided_orbit in command");

    // ========================================================================
    // Vehicle availability — Edit tools (all vehicles)
    // ========================================================================
    CHECK(updateItem.availableForVehicle("rotor"), "update for rotor");
    CHECK(updateItem.availableForVehicle("fixed_wing"), "update for fixed_wing");
    CHECK(updateItem.availableForVehicle("vtol"), "update for vtol");
    CHECK(updateItem.availableForVehicle("ground"), "update for ground");
    CHECK(deleteItem.availableForVehicle("rotor"), "delete for rotor");
    CHECK(moveItem.availableForVehicle("ground"), "move for ground");
    CHECK(reorderItem.availableForVehicle("vtol"), "reorder for vtol");

    // ========================================================================
    // Vehicle availability — Guided tools
    // ========================================================================
    // GuidedTakeoff: fixed_wing + vtol only
    CHECK(guidedTakeoff.availableForVehicle("fixed_wing"), "guided_takeoff for fixed_wing");
    CHECK(guidedTakeoff.availableForVehicle("vtol"), "guided_takeoff for vtol");
    CHECK(!guidedTakeoff.availableForVehicle("rotor"), "guided_takeoff NOT for rotor");
    CHECK(!guidedTakeoff.availableForVehicle("ground"), "guided_takeoff NOT for ground");

    // GuidedGoTo: all except ground/rover
    CHECK(guidedGoTo.availableForVehicle("rotor"), "guided_goto for rotor");
    CHECK(guidedGoTo.availableForVehicle("fixed_wing"), "guided_goto for fixed_wing");
    CHECK(!guidedGoTo.availableForVehicle("ground"), "guided_goto NOT for ground");
    CHECK(!guidedGoTo.availableForVehicle("rover"), "guided_goto NOT for rover");

    // GuidedRTL: all vehicles
    CHECK(guidedRTL.availableForVehicle("rotor"), "guided_rtl for rotor");
    CHECK(guidedRTL.availableForVehicle("ground"), "guided_rtl for ground");

    // GuidedLand: all vehicles
    CHECK(guidedLand.availableForVehicle("rotor"), "guided_land for rotor");
    CHECK(guidedLand.availableForVehicle("ground"), "guided_land for ground");

    // GuidedOrbit: rotor + vtol + multi_rotor only
    CHECK(guidedOrbit.availableForVehicle("rotor"), "guided_orbit for rotor");
    CHECK(guidedOrbit.availableForVehicle("vtol"), "guided_orbit for vtol");
    CHECK(guidedOrbit.availableForVehicle("multi_rotor"), "guided_orbit for multi_rotor");
    CHECK(!guidedOrbit.availableForVehicle("fixed_wing"), "guided_orbit NOT for fixed_wing");
    CHECK(!guidedOrbit.availableForVehicle("ground"), "guided_orbit NOT for ground");

    // ========================================================================
    // Registry integration — full 16-tool registry (7 add + 4 edit + 5 guided)
    // ========================================================================
    AgentToolRegistry registry;

    // Add tools (from existing test)
    registry.registerTool(new StubUpdateMissionItemTool(&registry));
    registry.registerTool(new StubDeleteMissionItemTool(&registry));
    registry.registerTool(new StubMoveItemTool(&registry));
    registry.registerTool(new StubReorderItemTool(&registry));
    registry.registerTool(new StubGuidedTakeoffTool(&registry));
    registry.registerTool(new StubGuidedGoToTool(&registry));
    registry.registerTool(new StubGuidedRTLTool(&registry));
    registry.registerTool(new StubGuidedLandTool(&registry));
    registry.registerTool(new StubGuidedOrbitTool(&registry));

    CHECK(registry.allTools().size() == 9, "registry has 9 edit+guided tools");

    // Full mode filtering: mission mode gets 4 edit tools; command gets 5 guided
    auto missionRotor = registry.getToolsForMode("mission", "rotor");
    CHECK(missionRotor.size() == 4, "mission+rotor: 4 edit tools");

    auto commandRotor = registry.getToolsForMode("command", "rotor");
    CHECK(commandRotor.size() == 4, "command+rotor: 4 guided (not takeoff)");

    auto commandFixedWing = registry.getToolsForMode("command", "fixed_wing");
    CHECK(commandFixedWing.size() == 4, "command+fixed_wing: 4 guided (takeoff+goto+rtl+land, not orbit)");

    auto commandGround = registry.getToolsForMode("command", "ground");
    CHECK(commandGround.size() == 2, "command+ground: 2 guided (rtl + land only)");

    // Tool definitions check
    QJsonArray defs = registry.getToolDefinitions("command", "rotor");
    CHECK(defs.size() >= 4, "command+rotor definitions has 4+ entries");
    bool allValid = true;
    for (const auto& val : defs) {
        auto obj = val.toObject();
        if (!obj.contains("name") || !obj.contains("parameters")) allValid = false;
    }
    CHECK(allValid, "all guided definitions have name and parameters");

    std::cout << "========================================" << std::endl;
    std::cout << "Edit & Guided Tools Schema Tests: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    return failed > 0 ? 1 : 0;
}