// Standalone test for mission add tools — schema and availability checks only.
// Uses stub tool classes (no execute() deps needed) so it can run headlessly
// in Docker without the full QGC codebase.
// Run in Docker: ./test_add_tools_schema

#include "AgentToolBase.h"
#include "AgentToolRegistry.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <iostream>
#include <memory>

// --- Stub tool classes with inline schema/availability (matching real tools) ---

class StubWaypointTool : public AgentToolBase {
public:
    explicit StubWaypointTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("add_waypoint"); }
    QString description() const override { return QStringLiteral("Add a waypoint to the mission."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["coordinates"] = QJsonObject{{"type", "string"}, {"description", "GPS coordinates as lat,lon"}};
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "Flight altitude in meters"}};
        props["altitude_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters", "feet"}}};
        props["seq"] = QJsonObject{{"type", "integer"}, {"description", "Insertion position (1-based)"}};
        props["distance"] = QJsonObject{{"type", "number"}, {"description", "Distance in meters for relative positioning"}};
        props["heading"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"north","northeast","east","southeast","south","southwest","west","northwest"}}};
        props["relative_reference_frame"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"origin","last_waypoint"}}};
        schema["properties"] = props;
        schema["required"] = QJsonArray();
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString&) const override { return true; }
    bool availableForVehicle(const QString&) const override { return true; }
};

class StubTakeoffTool : public AgentToolBase {
public:
    explicit StubTakeoffTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("add_takeoff"); }
    QString description() const override { return QStringLiteral("Add a takeoff command."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "Takeoff altitude in meters"}};
        props["altitude_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters","feet"}}};
        props["heading"] = QJsonObject{{"type", "string"}, {"description", "Takeoff heading in degrees"}};
        schema["properties"] = props;
        schema["required"] = QJsonArray();
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString&) const override { return true; }
    bool availableForVehicle(const QString& vt) const override {
        return vt == QStringLiteral("fixed_wing") || vt == QStringLiteral("vtol");
    }
};

class StubLandTool : public AgentToolBase {
public:
    explicit StubLandTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("add_land"); }
    QString description() const override { return QStringLiteral("Add a land command."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["coordinates"] = QJsonObject{{"type", "string"}, {"description", "GPS coordinates as lat,lon"}};
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "Landing altitude"}};
        schema["properties"] = props;
        schema["required"] = QJsonArray();
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString&) const override { return true; }
    bool availableForVehicle(const QString&) const override { return true; }
};

class StubLoiterTool : public AgentToolBase {
public:
    explicit StubLoiterTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("add_loiter"); }
    QString description() const override { return QStringLiteral("Add a loiter/orbit pattern."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["coordinates"] = QJsonObject{{"type", "string"}, {"description", "GPS coordinates"}};
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "Loiter altitude"}};
        props["radius"] = QJsonObject{{"type", "number"}, {"description", "Loiter radius in meters"}};
        props["radius_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters","feet"}}};
        props["seq"] = QJsonObject{{"type", "integer"}, {"description", "Insertion position"}};
        props["distance"] = QJsonObject{{"type", "number"}, {"description", "Relative distance"}};
        props["heading"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"north","east","south","west"}}};
        props["relative_reference_frame"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"origin","last_waypoint"}}};
        schema["properties"] = props;
        schema["required"] = QJsonArray();
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString&) const override { return true; }
    bool availableForVehicle(const QString& vt) const override { return vt != QStringLiteral("ground"); }
};

class StubRTLTool : public AgentToolBase {
public:
    explicit StubRTLTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("add_rtl"); }
    QString description() const override { return QStringLiteral("Add return-to-launch."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "RTL altitude"}};
        props["altitude_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters","feet"}}};
        schema["properties"] = props;
        schema["required"] = QJsonArray();
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString&) const override { return true; }
    bool availableForVehicle(const QString&) const override { return true; }
};

class StubSurveyTool : public AgentToolBase {
public:
    explicit StubSurveyTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("add_survey"); }
    QString description() const override { return QStringLiteral("Add a survey pattern."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["coordinates"] = QJsonObject{{"type", "string"}, {"description", "Survey center coordinates"}};
        props["altitude"] = QJsonObject{{"type", "number"}, {"description", "Survey altitude"}};
        props["altitude_units"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"meters","feet"}}};
        props["seq"] = QJsonObject{{"type", "integer"}, {"description", "Insertion position"}};
        props["distance"] = QJsonObject{{"type", "number"}, {"description", "Relative distance"}};
        props["heading"] = QJsonObject{{"type", "string"}, {"description", "Heading"}};
        props["relative_reference_frame"] = QJsonObject{{"type", "string"}, {"description", "Reference frame"}};
        schema["properties"] = props;
        schema["required"] = QJsonArray();
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString& mode) const override { return mode == QStringLiteral("mission"); }
    bool availableForVehicle(const QString& vt) const override {
        return vt == QStringLiteral("fixed_wing") || vt == QStringLiteral("rotor");
    }
};

class StubTransitionTool : public AgentToolBase {
public:
    explicit StubTransitionTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return QStringLiteral("add_transition"); }
    QString description() const override { return QStringLiteral("Add VTOL transition."); }
    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = QStringLiteral("object");
        QJsonObject props;
        props["target_state"] = QJsonObject{{"type", "string"}, {"enum", QJsonArray{"plane","hover"}}, {"description", "Target state"}};
        props["seq"] = QJsonObject{{"type", "integer"}, {"description", "Insertion position"}};
        schema["properties"] = props;
        QJsonArray required;
        required.append(QStringLiteral("target_state"));
        schema["required"] = required;
        return schema;
    }
    QString execute(const QJsonObject&) override { return {}; }
    bool availableInMode(const QString&) const override { return true; }
    bool availableForVehicle(const QString& vt) const override { return vt == QStringLiteral("vtol"); }
};

// --- Test harness ---

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

    // Create tool instances (local variables, pass by reference)
    StubWaypointTool waypoint;
    StubTakeoffTool takeoff;
    StubLandTool land;
    StubLoiterTool loiter;
    StubRTLTool rtl;
    StubSurveyTool survey;
    StubTransitionTool transition;

    // --- Tool names ---
    CHECK(waypoint.name() == "add_waypoint", "name: add_waypoint");
    CHECK(takeoff.name() == "add_takeoff", "name: add_takeoff");
    CHECK(land.name() == "add_land", "name: add_land");
    CHECK(loiter.name() == "add_loiter", "name: add_loiter");
    CHECK(rtl.name() == "add_rtl", "name: add_rtl");
    CHECK(survey.name() == "add_survey", "name: add_survey");
    CHECK(transition.name() == "add_transition", "name: add_transition");

    // --- Schema spot-checks ---
    CHECK(toolProperties(waypoint).contains("coordinates"), "waypoint has coordinates");
    CHECK(toolProperties(waypoint).contains("altitude"), "waypoint has altitude");
    CHECK(toolRequired(waypoint).size() == 0, "waypoint: 0 required");

    CHECK(toolProperties(takeoff).contains("altitude"), "takeoff has altitude");
    CHECK(toolRequired(takeoff).size() == 0, "takeoff: 0 required");

    CHECK(toolProperties(loiter).contains("radius"), "loiter has radius");
    CHECK(toolRequired(loiter).size() == 0, "loiter: 0 required");

    CHECK(toolProperties(rtl).contains("altitude"), "rtl has altitude");
    CHECK(toolRequired(rtl).size() == 0, "rtl: 0 required");

    CHECK(toolProperties(survey).contains("coordinates"), "survey has coordinates");
    CHECK(toolRequired(survey).size() == 0, "survey: 0 required");

    CHECK(toolProperties(transition).contains("target_state"), "transition has target_state");
    CHECK(toolRequired(transition).size() >= 1, "transition: 1+ required");

    // --- Mode availability ---
    CHECK(waypoint.availableInMode("mission"), "waypoint in mission");
    CHECK(waypoint.availableInMode("command"), "waypoint in command");
    CHECK(survey.availableInMode("mission"), "survey in mission");
    CHECK(!survey.availableInMode("command"), "survey NOT in command");
    CHECK(transition.availableInMode("mission"), "transition in mission");
    CHECK(transition.availableInMode("command"), "transition in command");

    // --- Vehicle availability ---
    CHECK(waypoint.availableForVehicle("rotor"), "waypoint for rotor");
    CHECK(waypoint.availableForVehicle("ground"), "waypoint for ground");
    CHECK(!takeoff.availableForVehicle("rotor"), "takeoff NOT for rotor");
    CHECK(takeoff.availableForVehicle("fixed_wing"), "takeoff for fixed_wing");
    CHECK(takeoff.availableForVehicle("vtol"), "takeoff for vtol");
    CHECK(loiter.availableForVehicle("rotor"), "loiter for rotor");
    CHECK(!loiter.availableForVehicle("ground"), "loiter NOT for ground");
    CHECK(survey.availableForVehicle("rotor"), "survey for rotor");
    CHECK(!survey.availableForVehicle("ground"), "survey NOT for ground");
    CHECK(!transition.availableForVehicle("rotor"), "transition NOT for rotor");
    CHECK(transition.availableForVehicle("vtol"), "transition for vtol");

    // --- Registry integration ---
    AgentToolRegistry registry;
    registry.registerTool(new StubWaypointTool(&registry));
    registry.registerTool(new StubTakeoffTool(&registry));
    registry.registerTool(new StubLandTool(&registry));
    registry.registerTool(new StubLoiterTool(&registry));
    registry.registerTool(new StubRTLTool(&registry));
    registry.registerTool(new StubSurveyTool(&registry));
    registry.registerTool(new StubTransitionTool(&registry));

    CHECK(registry.allTools().size() == 7, "registry has 7 tools");

    auto missionRotor = registry.getToolsForMode("mission", "rotor");
    CHECK(missionRotor.size() == 5, "mission+rotor: 5 tools");

    auto cmdVtol = registry.getToolsForMode("command", "vtol");
    CHECK(cmdVtol.size() == 6, "command+vtol: 6 tools");

    auto cmdGround = registry.getToolsForMode("command", "ground");
    CHECK(cmdGround.size() == 3, "command+ground: 3 tools");

    QJsonArray defs = registry.getToolDefinitions("mission", "rotor");
    CHECK(defs.size() >= 5, "definitions array has 5+ entries");
    bool allValid = true;
    for (const auto& val : defs) {
        auto obj = val.toObject();
        if (!obj.contains("name") || !obj.contains("parameters")) allValid = false;
    }
    CHECK(allValid, "all definitions have name and parameters");

    std::cout << "========================================" << std::endl;
    std::cout << "Add Tools Schema Tests: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    return failed > 0 ? 1 : 0;
}