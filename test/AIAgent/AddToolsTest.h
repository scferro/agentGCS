#pragma once

#include "UnitTest.h"
#include "AgentToolRegistry.h"

/// @brief Tests for the 7 Mission Add tools.
///
/// Validates tool names, JSON parameter schemas, availability mode/vehicle
/// filtering, and the registerAllTools() bulk registration function.
/// Does NOT call execute() — no vehicle or mission controller required.
class AddToolsTest : public UnitTest
{
    Q_OBJECT

public:
    AddToolsTest() = default;

private slots:
    void _waypointSchemaFields();
    void _takeoffSchemaFields();
    void _landSchemaFields();
    void _loiterSchemaFields();
    void _surveySchemaFields();
    void _rtlNoParameters();
    void _transitionSchemaFields();

    void _toolNames();
    void _toolAvailabilityByMode();
    void _toolAvailabilityByVehicle();
    void _registerAllTools();
    void _registryFiltersWithRealTools();
    void _toolDefinitionsArray();
};