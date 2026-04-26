#include "AddToolsTest.h"
#include "AddWaypointTool.h"
#include "AddTakeoffTool.h"
#include "AddLandTool.h"
#include "AddLoiterTool.h"
#include "AddRTLTool.h"
#include "AddSurveyTool.h"
#include "AddTransitionTool.h"
#include "RegisterTools.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

//-----------------------------------------------------------------------------
// Schema field helpers
//-----------------------------------------------------------------------------

/// Return the "properties" object from a tool's JSON schema.
static QJsonObject toolProperties(AgentToolBase* tool)
{
    auto schema = tool->parameters();
    return schema.value("properties").toObject();
}

/// Return the "required" array from a tool's JSON schema.
static QJsonArray toolRequired(AgentToolBase* tool)
{
    auto schema = tool->parameters();
    return schema.value("required").toArray();
}

//-----------------------------------------------------------------------------
// Schema field tests
//-----------------------------------------------------------------------------

void AddToolsTest::_waypointSchemaFields()
{
    AddWaypointTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("latitude"));
    QVERIFY(props.contains("longitude"));
    QVERIFY(props.contains("altitude"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 3);
}

void AddToolsTest::_takeoffSchemaFields()
{
    AddTakeoffTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("altitude"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 1);
}

void AddToolsTest::_landSchemaFields()
{
    AddLandTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("latitude"));
    QVERIFY(props.contains("longitude"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 2);
}

void AddToolsTest::_loiterSchemaFields()
{
    AddLoiterTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("latitude"));
    QVERIFY(props.contains("longitude"));
    QVERIFY(props.contains("altitude"));
    QVERIFY(props.contains("radius"));
    QVERIFY(props.contains("turns"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 5);
}

void AddToolsTest::_surveySchemaFields()
{
    AddSurveyTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("points"));
    QVERIFY(props.contains("camera_trigger_distance"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 2);
}

void AddToolsTest::_rtlNoParameters()
{
    AddRTLTool tool;
    auto schema = tool.parameters();
    // RTL has no input parameters — schema should be empty or have empty properties
    QVERIFY(schema.isEmpty() || schema.value("properties").toObject().isEmpty());
}

void AddToolsTest::_transitionSchemaFields()
{
    AddTransitionTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("state"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 1);
}

//-----------------------------------------------------------------------------
// Name, mode, vehicle tests
//-----------------------------------------------------------------------------

void AddToolsTest::_toolNames()
{
    QCOMPARE(QString(AddWaypointTool().name()),   QString("add_waypoint"));
    QCOMPARE(QString(AddTakeoffTool().name()),    QString("add_takeoff"));
    QCOMPARE(QString(AddLandTool().name()),       QString("add_land"));
    QCOMPARE(QString(AddLoiterTool().name()),     QString("add_loiter"));
    QCOMPARE(QString(AddRTLTool().name()),         QString("add_rtl"));
    QCOMPARE(QString(AddSurveyTool().name()),      QString("add_survey"));
    QCOMPARE(QString(AddTransitionTool().name()),  QString("add_transition"));
}

void AddToolsTest::_toolAvailabilityByMode()
{
    // Mission-mode tools
    QVERIFY(AddWaypointTool().availableInMode("mission"));
    QVERIFY(AddTakeoffTool().availableInMode("mission"));
    QVERIFY(AddLandTool().availableInMode("mission"));
    QVERIFY(AddLoiterTool().availableInMode("mission"));
    QVERIFY(AddRTLTool().availableInMode("mission"));
    QVERIFY(AddSurveyTool().availableInMode("mission"));

    // Transition is command-mode only
    QVERIFY(!AddTransitionTool().availableInMode("mission"));
    QVERIFY(AddTransitionTool().availableInMode("command"));

    // Most mission tools are NOT available in command mode
    QVERIFY(!AddWaypointTool().availableInMode("command"));
    QVERIFY(!AddSurveyTool().availableInMode("command"));
}

void AddToolsTest::_toolAvailabilityByVehicle()
{
    // All mission tools available for generic/rotor
    QVERIFY(AddWaypointTool().availableForVehicle("rotor"));
    QVERIFY(AddTakeoffTool().availableForVehicle("rotor"));
    QVERIFY(AddLandTool().availableForVehicle("rotor"));

    // Fixed-wing specifics: takeoff available, land available
    QVERIFY(AddTakeoffTool().availableForVehicle("fixed_wing"));
    QVERIFY(AddLandTool().availableForVehicle("fixed_wing"));

    // VTOL transition only for VTOL
    QVERIFY(AddTransitionTool().availableForVehicle("vtol"));
    QVERIFY(!AddTransitionTool().availableForVehicle("rotor"));
    QVERIFY(!AddTransitionTool().availableForVehicle("fixed_wing"));
}

//-----------------------------------------------------------------------------
// Registration & registry tests
//-----------------------------------------------------------------------------

void AddToolsTest::_registerAllTools()
{
    AgentToolRegistry registry;
    registerAllTools(&registry);
    // Should register all 7 tools
    QVERIFY(registry.allTools().size() >= 7);
}

void AddToolsTest::_registryFiltersWithRealTools()
{
    AgentToolRegistry registry;
    registerAllTools(&registry);

    // Mission-mode, rotor vehicle: 6 tools (all except transition)
    auto missionRotor = registry.getToolsForMode("mission", "rotor");
    QVERIFY(missionRotor.size() >= 6);

    // Command-mode, vtol: at least transition
    auto cmdVtol = registry.getToolsForMode("command", "vtol");
    bool hasTransition = false;
    for (auto* t : cmdVtol) {
        if (t->name() == "add_transition") hasTransition = true;
    }
    QVERIFY(hasTransition);
}

void AddToolsTest::_toolDefinitionsArray()
{
    AgentToolRegistry registry;
    registerAllTools(&registry);

    QJsonArray defs = registry.getToolDefinitions("mission", "rotor");
    QVERIFY(defs.size() >= 6);

    // Each definition should have "name" and "parameters"
    for (const auto& val : defs) {
        auto obj = val.toObject();
        QVERIFY(obj.contains("name"));
        QVERIFY(obj.contains("parameters"));
    }
}

UT_REGISTER_TEST(AddToolsTest, TestLabel::Unit, TestLabel::MissionManager)