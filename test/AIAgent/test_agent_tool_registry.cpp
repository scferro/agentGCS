#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <iostream>

#include "AgentToolBase.h"
#include "AgentToolRegistry.h"

// ── Stub tools for testing ──

class AddWaypointTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit AddWaypointTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return "add_waypoint"; }
    QString description() const override { return "Add a waypoint to the mission."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"latitude", QJsonObject{{"type", "number"}, {"description", "Latitude in degrees"}}},
                {"longitude", QJsonObject{{"type", "number"}, {"description", "Longitude in degrees"}}},
                {"altitude", QJsonObject{{"type", "number"}, {"description", "Altitude in meters (relative)"}}}
            }},
            {"required", QJsonArray{"latitude", "longitude", "altitude"}}
        };
    }
    QString execute(const QJsonObject&) override { return "{\"status\": \"ok\"}"; }
    // Available in all modes and for all vehicles
};

class UpdateWaypointTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit UpdateWaypointTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return "update_waypoint"; }
    QString description() const override { return "Update an existing waypoint's properties."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"index", QJsonObject{{"type", "integer"}}},
                {"latitude", QJsonObject{{"type", "number"}}},
                {"longitude", QJsonObject{{"type", "number"}}}
            }},
            {"required", QJsonArray{"index"}}
        };
    }
    QString execute(const QJsonObject&) override { return "{\"status\": \"ok\"}"; }
    // Only available in mission mode (edit tool)
    bool availableInMode(const QString& mode) const override { return mode == "mission"; }
};

class AddLoiterTool : public AgentToolBase {
    Q_OBJECT
public:
    explicit AddLoiterTool(QObject* parent = nullptr) : AgentToolBase(parent) {}
    QString name() const override { return "add_loiter"; }
    QString description() const override { return "Add a loiter point to the mission."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"latitude", QJsonObject{{"type", "number"}}},
                {"longitude", QJsonObject{{"type", "number"}}},
                {"altitude", QJsonObject{{"type", "number"}}},
                {"radius", QJsonObject{{"type", "number"}}}
            }},
            {"required", QJsonArray{"latitude", "longitude", "altitude"}}
        };
    }
    QString execute(const QJsonObject&) override { return "{\"status\": \"ok\"}"; }
    // Not available for ground vehicles
    bool availableForVehicle(const QString& vehicleType) const override {
        return vehicleType != "ground" && vehicleType != "rover";
    }
};

// ── Test helpers ──

static int testsPassed = 0;
static int testsFailed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            qDebug() << "FAIL:" << message; \
            testsFailed++; \
        } else { \
            qDebug() << "PASS:" << message; \
            testsPassed++; \
        } \
    } while(0)

// ── Tests ──

void testRegisterAndGetAllTools()
{
    AgentToolRegistry registry;
    auto* addWp = new AddWaypointTool();
    auto* updateWp = new UpdateWaypointTool();
    auto* addLoiter = new AddLoiterTool();

    registry.registerTool(addWp);
    registry.registerTool(updateWp);
    registry.registerTool(addLoiter);

    QList<AgentToolBase*> allTools = registry.allTools();
    TEST_ASSERT(allTools.size() == 3, "allTools returns 3 registered tools");

    // Double-register should be ignored
    registry.registerTool(addWp);
    TEST_ASSERT(registry.allTools().size() == 3, "double-register ignored");

    // nullptr should be ignored
    registry.registerTool(nullptr);
    TEST_ASSERT(registry.allTools().size() == 3, "nullptr register ignored");
}

void testGetToolByName()
{
    AgentToolRegistry registry;
    auto* addWp = new AddWaypointTool();
    registry.registerTool(addWp);

    TEST_ASSERT(registry.getToolByName("add_waypoint") == addWp, "getToolByName returns correct tool");
    TEST_ASSERT(registry.getToolByName("nonexistent") == nullptr, "getToolByName returns nullptr for missing tool");
}

void testMissionModeGetsAllTools()
{
    AgentToolRegistry registry;
    auto* addWp = new AddWaypointTool();
    auto* updateWp = new UpdateWaypointTool();
    auto* addLoiter = new AddLoiterTool();

    registry.registerTool(addWp);
    registry.registerTool(updateWp);
    registry.registerTool(addLoiter);

    // Mission mode: all tools should be available for multi_rotor
    auto tools = registry.getToolsForMode("mission", "multi_rotor");
    TEST_ASSERT(tools.size() == 3, "mission+multi_rotor gets all 3 tools");

    // Verify names
    QStringList names;
    for (auto* t : tools) names << t->name();
    TEST_ASSERT(names.contains("add_waypoint"), "mission mode includes add_waypoint");
    TEST_ASSERT(names.contains("update_waypoint"), "mission mode includes update_waypoint");
    TEST_ASSERT(names.contains("add_loiter"), "mission mode includes add_loiter");
}

void testCommandModeExcludesEditTools()
{
    AgentToolRegistry registry;
    auto* addWp = new AddWaypointTool();
    auto* updateWp = new UpdateWaypointTool();
    auto* addLoiter = new AddLoiterTool();

    registry.registerTool(addWp);
    registry.registerTool(updateWp);
    registry.registerTool(addLoiter);

    // Command mode: update_waypoint should be excluded (it requires "mission" mode)
    auto tools = registry.getToolsForMode("command", "multi_rotor");
    TEST_ASSERT(tools.size() == 2, "command+multi_rotor gets 2 tools (no edit)");

    QStringList names;
    for (auto* t : tools) names << t->name();
    TEST_ASSERT(names.contains("add_waypoint"), "command mode includes add_waypoint");
    TEST_ASSERT(names.contains("add_loiter"), "command mode includes add_loiter");
    TEST_ASSERT(!names.contains("update_waypoint"), "command mode excludes update_waypoint");
}

void testGroundVehicleExcludesLoiter()
{
    AgentToolRegistry registry;
    auto* addWp = new AddWaypointTool();
    auto* addLoiter = new AddLoiterTool();

    registry.registerTool(addWp);
    registry.registerTool(addLoiter);

    // Ground vehicle: loiter should be excluded
    auto tools = registry.getToolsForMode("command", "ground");
    TEST_ASSERT(tools.size() == 1, "command+ground gets 1 tool (no loiter)");

    QStringList names;
    for (auto* t : tools) names << t->name();
    TEST_ASSERT(names.contains("add_waypoint"), "ground includes add_waypoint");
    TEST_ASSERT(!names.contains("add_loiter"), "ground excludes add_loiter");

    // Rover: same as ground
    tools = registry.getToolsForMode("command", "rover");
    TEST_ASSERT(tools.size() == 1, "command+rover gets 1 tool (no loiter)");
}

void testGetToolDefinitionsReturnsValidJsonSchema()
{
    AgentToolRegistry registry;
    auto* addWp = new AddWaypointTool();
    registry.registerTool(addWp);

    QJsonArray defs = registry.getToolDefinitions("mission", "multi_rotor");
    TEST_ASSERT(defs.size() == 1, "getToolDefinitions returns 1 definition");

    QJsonObject def = defs[0].toObject();
    TEST_ASSERT(def["name"].toString() == "add_waypoint", "definition has correct name");
    TEST_ASSERT(def["description"].toString() == "Add a waypoint to the mission.", "definition has description");
    TEST_ASSERT(def.contains("parameters"), "definition has parameters");

    QJsonObject params = def["parameters"].toObject();
    TEST_ASSERT(params["type"].toString() == "object", "parameters type is object");
    TEST_ASSERT(params.contains("properties"), "parameters has properties");
    TEST_ASSERT(params.contains("required"), "parameters has required");

    QJsonObject properties = params["properties"].toObject();
    TEST_ASSERT(properties.contains("latitude"), "properties has latitude");
    TEST_ASSERT(properties.contains("longitude"), "properties has longitude");
    TEST_ASSERT(properties.contains("altitude"), "properties has altitude");
}

void testToolDefinitionOnBase()
{
    AddWaypointTool tool;
    QJsonObject def = tool.toolDefinition();
    TEST_ASSERT(def["name"].toString() == "add_waypoint", "toolDefinition name matches");
    TEST_ASSERT(def.contains("parameters"), "toolDefinition has parameters");
    TEST_ASSERT(def["parameters"].toObject()["type"].toString() == "object", "toolDefinition parameters type");
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    testRegisterAndGetAllTools();
    testGetToolByName();
    testMissionModeGetsAllTools();
    testCommandModeExcludesEditTools();
    testGroundVehicleExcludesLoiter();
    testGetToolDefinitionsReturnsValidJsonSchema();
    testToolDefinitionOnBase();

    qDebug() << "========================================";
    qDebug() << "Tool Registry Tests:" << testsPassed << "passed," << testsFailed << "failed";
    qDebug() << "========================================";

    return testsFailed > 0 ? 1 : 0;
}

#include "test_agent_tool_registry.moc"