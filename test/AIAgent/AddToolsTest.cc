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
// Schema field tests — verify properties match actual tool schemas
//-----------------------------------------------------------------------------

void AddToolsTest::_waypointSchemaFields()
{
    AddWaypointTool tool;
    auto props = toolProperties(&tool);
    // AddWaypointTool uses "coordinates" (lat,lon string), not separate lat/lon
    QVERIFY(props.contains("coordinates"));
    QVERIFY(props.contains("altitude"));
    QVERIFY(props.contains("altitude_units"));
    QVERIFY(props.contains("seq"));
    QVERIFY(props.contains("distance"));
    QVERIFY(props.contains("heading"));
    QVERIFY(props.contains("relative_reference_frame"));
    // No required fields — coordinates OR relative positioning
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 0);
}

void AddToolsTest::_takeoffSchemaFields()
{
    AddTakeoffTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("altitude"));
    QVERIFY(props.contains("altitude_units"));
    QVERIFY(props.contains("heading"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 0);
}

void AddToolsTest::_landSchemaFields()
{
    AddLandTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("coordinates"));
    QVERIFY(props.contains("altitude"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 0);
}

void AddToolsTest::_loiterSchemaFields()
{
    AddLoiterTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("coordinates"));
    QVERIFY(props.contains("altitude"));
    QVERIFY(props.contains("radius"));
    QVERIFY(props.contains("radius_units"));
    QVERIFY(props.contains("seq"));
    QVERIFY(props.contains("distance"));
    QVERIFY(props.contains("heading"));
    QVERIFY(props.contains("relative_reference_frame"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 0);
}

void AddToolsTest::_surveySchemaFields()
{
    AddSurveyTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("coordinates"));
    QVERIFY(props.contains("altitude"));
    QVERIFY(props.contains("altitude_units"));
    QVERIFY(props.contains("seq"));
    QVERIFY(props.contains("distance"));
    QVERIFY(props.contains("heading"));
    QVERIFY(props.contains("relative_reference_frame"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 0);
}

void AddToolsTest::_rtlNoParameters()
{
    AddRTLTool tool;
    auto schema = tool.parameters();
    // RTL has altitude and altitude_units but no required fields
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("altitude"));
    QVERIFY(props.contains("altitude_units"));
    auto req = toolRequired(&tool);
    QCOMPARE(req.size(), 0);
}

void AddToolsTest::_transitionSchemaFields()
{
    AddTransitionTool tool;
    auto props = toolProperties(&tool);
    QVERIFY(props.contains("target_state"));
    QVERIFY(props.contains("seq"));
    auto req = toolRequired(&tool);
    QVERIFY(req.size() >= 1); // target_state is required
}

//-----------------------------------------------------------------------------
// Name, mode, vehicle tests
//-----------------------------------------------------------------------------

void AddToolsTest::_toolNames()
{
    QCOMPARE(QString(AddWaypointTool().name()),   QString("add_waypoint"));
    QCOMPARE(QString(AddTakeoffTool().name()),    QString("add_takeoff"));
    QCOMPARE(QString(AddLandTool().name()),        QString("add_land"));
    QCOMPARE(QString(AddLoiterTool().name()),      QString("add_loiter"));
    QCOMPARE(QString(AddRTLTool().name()),         QString("add_rtl"));
    QCOMPARE(QString(AddSurveyTool().name()),      QString("add_survey"));
    QCOMPARE(QString(AddTransitionTool().name()),  QString("add_transition"));
}

void AddToolsTest::_toolAvailabilityByMode()
{
    // Most add tools are available in both mission and command modes
    QVERIFY(AddWaypointTool().availableInMode("mission"));
    QVERIFY(AddWaypointTool().availableInMode("command"));
    QVERIFY(AddTakeoffTool().availableInMode("mission"));
    QVERIFY(AddTakeoffTool().availableInMode("command"));
    QVERIFY(AddLandTool().availableInMode("mission"));
    QVERIFY(AddLandTool().availableInMode("command"));
    QVERIFY(AddLoiterTool().availableInMode("mission"));
    QVERIFY(AddLoiterTool().availableInMode("command"));
    QVERIFY(AddRTLTool().availableInMode("mission"));
    QVERIFY(AddRTLTool().availableInMode("command"));
    QVERIFY(AddTransitionTool().availableInMode("mission"));
    QVERIFY(AddTransitionTool().availableInMode("command"));

    // Survey is mission-mode only (requires complex mission item setup)
    QVERIFY(AddSurveyTool().availableInMode("mission"));
    QVERIFY(!AddSurveyTool().availableInMode("command"));
}

void AddToolsTest::_toolAvailabilityByVehicle()
{
    // Waypoint: all vehicles
    QVERIFY(AddWaypointTool().availableForVehicle("rotor"));
    QVERIFY(AddWaypointTool().availableForVehicle("fixed_wing"));
    QVERIFY(AddWaypointTool().availableForVehicle("ground"));
    QVERIFY(AddWaypointTool().availableForVehicle("vtol"));

    // Takeoff: fixed_wing + vtol only (rotors take off vertically)
    QVERIFY(!AddTakeoffTool().availableForVehicle("rotor"));
    QVERIFY(AddTakeoffTool().availableForVehicle("fixed_wing"));
    QVERIFY(!AddTakeoffTool().availableForVehicle("ground"));
    QVERIFY(AddTakeoffTool().availableForVehicle("vtol"));

    // Land: all vehicles
    QVERIFY(AddLandTool().availableForVehicle("rotor"));
    QVERIFY(AddLandTool().availableForVehicle("fixed_wing"));
    QVERIFY(AddLandTool().availableForVehicle("ground"));

    // Loiter: all EXCEPT ground
    QVERIFY(AddLoiterTool().availableForVehicle("rotor"));
    QVERIFY(AddLoiterTool().availableForVehicle("fixed_wing"));
    QVERIFY(!AddLoiterTool().availableForVehicle("ground"));
    QVERIFY(AddLoiterTool().availableForVehicle("vtol"));

    // Survey: fixed_wing + multi_rotor only
    QVERIFY(AddSurveyTool().availableForVehicle("rotor"));
    QVERIFY(AddSurveyTool().availableForVehicle("fixed_wing"));
    QVERIFY(!AddSurveyTool().availableForVehicle("ground"));
    QVERIFY(!AddSurveyTool().availableForVehicle("vtol"));

    // VTOL transition: only VTOL
    QVERIFY(!AddTransitionTool().availableForVehicle("rotor"));
    QVERIFY(!AddTransitionTool().availableForVehicle("fixed_wing"));
    QVERIFY(AddTransitionTool().availableForVehicle("vtol"));
}

//-----------------------------------------------------------------------------
// Registration & registry tests
//-----------------------------------------------------------------------------

void AddToolsTest::_registerAllTools()
{
    AgentToolRegistry registry;
    registerAllTools(&registry);
    QCOMPARE(registry.allTools().size(), 7);
}

void AddToolsTest::_registryFiltersWithRealTools()
{
    AgentToolRegistry registry;
    registerAllTools(&registry);

    // Mission-mode, rotor vehicle: waypoint, takeoff(no), land, loiter, rtl, survey, transition(no) = 5
    auto missionRotor = registry.getToolsForMode("mission", "rotor");
    QCOMPARE(missionRotor.size(), 5);

    // Command-mode, vtol: waypoint, takeoff, land, loiter, rtl, transition = 6 (no survey)
    auto cmdVtol = registry.getToolsForMode("command", "vtol");
    QCOMPARE(cmdVtol.size(), 6);

    // Command-mode, ground: waypoint, land, rtl = 3 (no takeoff, no loiter, no survey, no transition)
    auto cmdGround = registry.getToolsForMode("command", "ground");
    QCOMPARE(cmdGround.size(), 3);
}

void AddToolsTest::_toolDefinitionsArray()
{
    AgentToolRegistry registry;
    registerAllTools(&registry);

    QJsonArray defs = registry.getToolDefinitions("mission", "rotor");
    QVERIFY(defs.size() >= 5);

    // Each definition should have "name" and "parameters"
    for (const auto& val : defs) {
        auto obj = val.toObject();
        QVERIFY(obj.contains("name"));
        QVERIFY(obj.contains("parameters"));
    }
}

UT_REGISTER_TEST(AddToolsTest, TestLabel::Unit, TestLabel::MissionManager)