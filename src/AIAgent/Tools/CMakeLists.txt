# Mission Add Tool implementations.
#
# Tool sources are added to the main QGC executable (${CMAKE_PROJECT_NAME})
# because they reference QGC subsystem classes (MissionController, Vehicle,
# Fact, SimpleMissionItem, etc.) whose symbols live only in the final
# executable. AIAgentModule is a separate STATIC library and cannot resolve
# those symbols at link time.
#
# IMPORTANT: Tool headers are NOT added to AIAgentModule. If they were,
# AIAgentModule's AUTOMOC would generate moc_*.cpp files that reference the
# tool virtual methods (execute, parameters, description). But those methods
# are defined in the tool .cpp files compiled into the main executable.
# AIAgentModule (a STATIC lib) can't resolve them, causing linker errors.
# Instead, the main QGC target's AUTOMOC handles the tool headers.

# Tool implementations AND headers compiled into the main QGC executable.
# AUTOMOC for the main target will process the .h files and generate the
# corresponding moc_*.cpp files, which get compiled alongside the .cpp files.
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/AddWaypointTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AddWaypointTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AddTakeoffTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AddTakeoffTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AddLandTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AddLandTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AddLoiterTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AddLoiterTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AddRTLTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AddRTLTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AddSurveyTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AddSurveyTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AddTransitionTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AddTransitionTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/UpdateMissionItemTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/UpdateMissionItemTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DeleteMissionItemTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/DeleteMissionItemTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/MoveItemTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/MoveItemTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ReorderItemTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ReorderItemTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedTakeoffTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedTakeoffTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedGoToTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedGoToTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedRTLTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedRTLTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedLandTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedLandTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedOrbitTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/GuidedOrbitTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RegisterTools.h
        ${CMAKE_CURRENT_SOURCE_DIR}/RegisterTools.cpp
)

# Main target needs the Tools/ include path for tool headers
target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)