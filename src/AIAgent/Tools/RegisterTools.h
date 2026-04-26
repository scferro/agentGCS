#pragma once

class AgentToolRegistry;

/// Instantiate and register all mission add tools with the given registry.
/// Call this once during application startup (e.g., from AgentController constructor).
void registerAllTools(AgentToolRegistry* registry);