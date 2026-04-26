// =============================================================================
// test_context_injection.cpp
// Standalone tests for AgentPrompts and AgentContextSerializer.
// Tests that system prompts, vehicle variants, and context JSON are correct.
// No QGC deps — runs headlessly in Docker.
// =============================================================================

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <iostream>

#include "AgentPrompts.h"
#include "AgentContextSerializer.h"

static int testsPassed = 0;
static int testsFailed = 0;

#define ASSERT_TRUE(expr, msg) \
    do { \
        if (!(expr)) { \
            std::cerr << "  FAIL: " << msg << " (line " << __LINE__ << ")" << std::endl; \
            testsFailed++; \
        } else { \
            std::cerr << "  PASS: " << msg << std::endl; \
            testsPassed++; \
        } \
    } while (0)

#define ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "  FAIL: " << msg << " (line " << __LINE__ << ")" \
                      << " — got: " << (a).toStdString() \
                      << ", expected: " << (b).toStdString() << std::endl; \
            testsFailed++; \
        } else { \
            std::cerr << "  PASS: " << msg << std::endl; \
            testsPassed++; \
        } \
    } while (0)

#define ASSERT_CONTAINS(haystack, needle, msg) \
    do { \
        if (!(haystack).contains(QString(needle))) { \
            std::cerr << "  FAIL: " << msg << " (line " << __LINE__ << ")" \
                      << " — string does not contain: " << (needle) << std::endl; \
            testsFailed++; \
        } else { \
            std::cerr << "  PASS: " << msg << std::endl; \
            testsPassed++; \
        } \
    } while (0)

// =============================================================================
// AgentPrompts tests
// =============================================================================

void testMissionPromptIsNonEmpty()
{
    QString prompt = missionSystemPrompt();
    ASSERT_TRUE(!prompt.isEmpty(), "Mission system prompt is non-empty");
}

void testCommandPromptIsNonEmpty()
{
    QString prompt = commandSystemPrompt();
    ASSERT_TRUE(!prompt.isEmpty(), "Command system prompt is non-empty");
}

void testMissionPromptContainsDroneMission()
{
    QString prompt = missionSystemPrompt();
    ASSERT_CONTAINS(prompt, "mission", "Mission prompt contains 'mission'");
    ASSERT_CONTAINS(prompt, "tools", "Mission prompt mentions tools");
}

void testCommandPromptContainsDroneCommand()
{
    QString prompt = commandSystemPrompt();
    ASSERT_CONTAINS(prompt, "command", "Command prompt contains 'command'");
    ASSERT_CONTAINS(prompt, "tool", "Command prompt mentions tools");
}

void testMissionPromptHasNoThinkPrefix()
{
    QString prompt = missionSystemPrompt();
    ASSERT_TRUE(prompt.startsWith("/no_think"), "Mission prompt starts with /no_think");
}

void testCommandPromptHasNoThinkPrefix()
{
    QString prompt = commandSystemPrompt();
    ASSERT_TRUE(prompt.startsWith("/no_think"), "Command prompt starts with /no_think");
}

void testBuildSystemPromptMissionMode()
{
    QString prompt = buildSystemPrompt("mission", "multicopter");
    ASSERT_CONTAINS(prompt, "mission", "Built prompt in mission mode contains 'mission'");
    ASSERT_CONTAINS(prompt, "Multirotor", "Built prompt in mission mode with multicopter has vehicle addition");
}

void testBuildSystemPromptCommandMode()
{
    QString prompt = buildSystemPrompt("command", "multicopter");
    ASSERT_CONTAINS(prompt, "command", "Built prompt in command mode contains 'command'");
    ASSERT_CONTAINS(prompt, "Multirotor", "Built prompt in command mode with multicopter has vehicle addition");
}

// =============================================================================
// Vehicle prompt variant tests
// =============================================================================

void testFixedWingVehiclePrompt()
{
    QString addition = vehiclePromptAddition("fixed_wing");
    ASSERT_CONTAINS(addition, "Fixed-wing", "Fixed-wing vehicle prompt contains 'Fixed-wing'");
    ASSERT_CONTAINS(addition, "Takeoff requires a heading", "Fixed-wing prompt mentions takeoff heading");
}

void testVTOLVehiclePrompt()
{
    QString addition = vehiclePromptAddition("vtol");
    ASSERT_CONTAINS(addition, "VTOL", "VTOL vehicle prompt contains 'VTOL'");
    ASSERT_CONTAINS(addition, "transition", "VTOL prompt mentions transition");
}

void testRoverVehiclePrompt()
{
    QString addition = vehiclePromptAddition("rover");
    ASSERT_CONTAINS(addition, "rover", "Rover vehicle prompt contains 'rover'");
    ASSERT_CONTAINS(addition, "No altitude", "Rover prompt mentions no altitude");
}

void testGroundVehiclePrompt()
{
    QString addition = vehiclePromptAddition("ground");
    ASSERT_CONTAINS(addition, "Ground rover", "Ground vehicle prompt contains 'Ground rover'");
}

void testMulticopterVehiclePrompt()
{
    QString addition = vehiclePromptAddition("multicopter");
    ASSERT_CONTAINS(addition, "Multirotor", "Multicopter vehicle prompt contains 'Multirotor'");
    ASSERT_CONTAINS(addition, "hover", "Multicopter prompt mentions hover");
}

void testUnknownVehicleFallsBackToMulticopter()
{
    QString unknown = vehiclePromptAddition("unknown_type");
    QString multi = vehiclePromptAddition("multicopter");
    ASSERT_EQ(unknown, multi, "Unknown vehicle type falls back to multicopter prompt");
}

// =============================================================================
// AgentContextSerializer tests
// =============================================================================

void testMissionStateJsonHasExpectedKeys()
{
    QString json = serializeMissionState();
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    ASSERT_TRUE(doc.isObject(), "Mission state JSON is valid object");

    QJsonObject obj = doc.object();
    ASSERT_TRUE(obj.contains("total_mission_items"), "Mission state has total_mission_items");
    ASSERT_TRUE(obj.contains("home_position"), "Mission state has home_position");
    ASSERT_TRUE(obj.contains("mission_state"), "Mission state has mission_state");
}

void testMissionStateHomePositionHasLatLonAlt()
{
    QString json = serializeMissionState();
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = doc.object();
    ASSERT_TRUE(obj["home_position"].isObject(), "home_position is object");

    QJsonObject home = obj["home_position"].toObject();
    ASSERT_TRUE(home.contains("lat"), "home_position has lat");
    ASSERT_TRUE(home.contains("lon"), "home_position has lon");
    ASSERT_TRUE(home.contains("alt"), "home_position has alt");
}

void testVehicleStateJsonHasExpectedKeys()
{
    QString json = serializeVehicleState();
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    ASSERT_TRUE(doc.isObject(), "Vehicle state JSON is valid object");

    QJsonObject obj = doc.object();
    ASSERT_TRUE(obj.contains("lat"), "Vehicle state has lat");
    ASSERT_TRUE(obj.contains("lon"), "Vehicle state has lon");
    ASSERT_TRUE(obj.contains("alt"), "Vehicle state has alt");
    ASSERT_TRUE(obj.contains("armed"), "Vehicle state has armed");
    ASSERT_TRUE(obj.contains("flight_mode"), "Vehicle state has flight_mode");
    ASSERT_TRUE(obj.contains("battery_percent"), "Vehicle state has battery_percent");
    ASSERT_TRUE(obj.contains("gps_lock"), "Vehicle state has gps_lock");
    ASSERT_TRUE(obj.contains("heading"), "Vehicle state has heading");
}

void testVehicleStateArmedIsBool()
{
    QString json = serializeVehicleState();
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = doc.object();
    ASSERT_TRUE(obj["armed"].isBool(), "Vehicle armed is boolean");
}

void testVehicleStateFlightModeIsString()
{
    QString json = serializeVehicleState();
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = doc.object();
    ASSERT_TRUE(obj["flight_mode"].isString(), "Vehicle flight_mode is string");
}

// =============================================================================
// Mission vs command context message tests
// =============================================================================

void testMissionContextMessageStartsWithLabel()
{
    QString msg = missionContextMessage();
    ASSERT_CONTAINS(msg, "Current mission state", "Mission context message starts with label");
    ASSERT_CONTAINS(msg, "total_mission_items", "Mission context message contains JSON data");
}

void testVehicleContextMessageStartsWithLabel()
{
    QString msg = vehicleContextMessage();
    ASSERT_CONTAINS(msg, "Current vehicle state", "Vehicle context message starts with label");
    ASSERT_CONTAINS(msg, "armed", "Vehicle context message contains armed key");
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    std::cerr << "\n=== AgentPrompts tests ===" << std::endl;
    testMissionPromptIsNonEmpty();
    testCommandPromptIsNonEmpty();
    testMissionPromptContainsDroneMission();
    testCommandPromptContainsDroneCommand();
    testMissionPromptHasNoThinkPrefix();
    testCommandPromptHasNoThinkPrefix();
    testBuildSystemPromptMissionMode();
    testBuildSystemPromptCommandMode();

    std::cerr << "\n=== Vehicle prompt variant tests ===" << std::endl;
    testFixedWingVehiclePrompt();
    testVTOLVehiclePrompt();
    testRoverVehiclePrompt();
    testGroundVehiclePrompt();
    testMulticopterVehiclePrompt();
    testUnknownVehicleFallsBackToMulticopter();

    std::cerr << "\n=== AgentContextSerializer tests ===" << std::endl;
    testMissionStateJsonHasExpectedKeys();
    testMissionStateHomePositionHasLatLonAlt();
    testVehicleStateJsonHasExpectedKeys();
    testVehicleStateArmedIsBool();
    testVehicleStateFlightModeIsString();

    std::cerr << "\n=== Context message tests ===" << std::endl;
    testMissionContextMessageStartsWithLabel();
    testVehicleContextMessageStartsWithLabel();

    std::cerr << "\n=== Results ===" << std::endl;
    std::cerr << "Passed: " << testsPassed << "  Failed: " << testsFailed << std::endl;

    return testsFailed > 0 ? 1 : 0;
}