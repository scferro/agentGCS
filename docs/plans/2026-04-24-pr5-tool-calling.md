# PR 5 — Tool Calling Support: Detailed Task Plan

**Branch:** `feat/ai-agent-pr5-tool-calling` (already created from `feat/ai-agent-pr1-llama-cmake`)
**Parent branch has:** PRs 1-4 committed + plan reorder commit

## Current State

✅ `LLMEngine.h` — has `AgentTool` struct, `setTools()`/`clearTools()`, `toolCallDetected` signal, `m_tools`, `m_lastChatParams`
✅ `LLMEngine.cpp` — tools passed to `common_chat_templates_inputs`, `common_chat_parse` on generated text, `toolCallDetected` emitted
✅ Build verifies clean in Docker — `AIAgentModule` (278KB), full QGC links
✅ QGC unit test `LLMEngineToolCallTest` compiles (5 unit tests + 3 integration tests)
✅ Standalone test `test_tool_call.cpp` compiles but has threading bug (being fixed)

⚠️ Standalone test threading bug: `moveToThread()` was wrong, already fixed to not use `moveToThread`
⚠️ Standalone test CMake target: currently hand-compiled with `g++`, needs nlohmann_json include from CPM — better as a CMake target
⚠️ No runtime validation yet — only build verification done

## Tasks

### Task 5.1: Fix standalone test threading + add as CMake target

**Problem:** The standalone test `test_tool_call.cpp` was hand-compiled with `g++` and had `moveToThread()` which caused `loadModel()` to fail from the wrong thread. The hand-compiled g++ command is fragile (missing nlohmann_json CPM include paths).

**Fix (already applied to source):** Removed `moveToThread`/`QThread workerThread` — `loadModel()` runs on main thread, `startCompletion()` internally dispatches via `QMetaObject::invokeMethod` already.

**New work needed:** Add `test_tool_call.cpp` as a CMake build target so CPM/llama includes resolve automatically. This also makes it buildable by `ninja` in Docker like everything else.

**Files:**
- Modify: `test/AIAgent/CMakeLists.txt` — add `test_tool_call` executable target
- `test_tool_call.cpp` — source already written, threading fix already applied

**Verification:** `ninja -C /project/build test_tool_call` succeeds in Docker

---

### Task 5.2: Docker build-verify PR 5 code

**Objective:** Full Docker rebuild to confirm everything compiles and links including the new CMake test target.

**Steps:**
1. Reconfigure CMake: `qt-cmake -S /project/source -B /project/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DQGC_BUILD_TESTING=ON`
2. Build: `ninja -C /project/build QGroundControlModule test_tool_call`
3. Verify: no compile/link errors

**Verification:** Clean build, all targets compile

---

### Task 5.3: Run standalone test with SmolLM2-135M

**Objective:** Quick smoke test with the tiny model (101MB, fast load/generate). SmolLM2 doesn't support tool calling, so we expect:
- ✅ Model loads successfully
- ✅ `--test register` passes (API-only test, no model needed)
- ✅ `--test notools` passes (generates text, no tool call signal)
- ⚠️ `--test tools` may not fire `toolCallDetected` (model has no tool support) — INFO, not FAIL

**Docker command:**
```bash
docker run --rm ... qgc-ubuntu-docker -l -c '
  /project/build/test_tool_call /models/SmolLM2-135M-Instruct-Q4_K_M.gguf --test register
  /project/build/test_tool_call /models/SmolLM2-135M-Instruct-Q4_K_M.gguf --test notools
  /project/build/test_tool_call /models/SmolLM2-135M-Instruct-Q4_K_M.gguf --test tools
'
```

**Verification:** register and notools tests pass; tools test completes without crash (INFO result is acceptable)

---

### Task 5.4: Run standalone test with Gemma 4 E2B

**Objective:** End-to-end tool calling validation with the real model. Gemma 4 E2B supports tool calling natively.

**Expected:**
- ✅ Model loads
- ✅ `--test tools` fires `toolCallDetected` with `get_weather` tool name
- ✅ `--test notools` generates text without tool call signal

**Note:** CPU-only in Docker, ~4.1 GiB RAM. Generation is slow (~1-2 min for a short prompt). This is the critical validation step.

**Docker command:**
```bash
docker run --rm ... qgc-ubuntu-docker -l -c '
  /project/build/test_tool_call /models/gemma-4-E2B-it-Q4_K_M.gguf --test tools
  /project/build/test_tool_call /models/gemma-4-E2B-it-Q4_K_M.gguf --test notools
'
```

**Verification:** `toolCallDetected` signal fires with correct tool name `get_weather`

---

### Task 5.5: Run QGC unit tests (no model required)

**Objective:** Run the 5 unit tests in `LLMEngineToolCallTest` that don't need a model file:
- `_setToolsTest` — setTools API works
- `_clearToolsTest` — clearTools API works
- `_toolRegistrationOverwriteTest` — setTools replaces, not appends
- `_toolCallDetectedSignatureTest` — signal exists with correct signature
- `_agentToolStructTest` — struct fields, copy, QList

**Docker command:**
```bash
docker run --rm ... qgc-ubuntu-docker -l -c '
  /project/build/Release/QGroundControl --unittest:LLMEngineToolCallTest
'
```

**Verification:** All 5 unit tests pass

---

### Task 5.6: Commit PR 5

**Objective:** Commit all PR 5 changes with a clear message.

**Files to commit:**
- `src/AIAgent/LLMEngine.h` — AgentTool struct, setTools/clearTools, toolCallDetected signal
- `src/AIAgent/LLMEngine.cpp` — tools in template inputs, common_chat_parse, toolCallDetected emission
- `test/AIAgent/LLMEngineToolCallTest.h/.cc` — QGC unit tests
- `test/AIAgent/test_tool_call.cpp` — standalone smoke test
- `test/AIAgent/CMakeLists.txt` — test sources + standalone test target
- `test/CMakeLists.txt` — LLMEngineToolCallTest registration

**Commit:**
```bash
git add src/AIAgent/LLMEngine.h src/AIAgent/LLMEngine.cpp \
        test/AIAgent/LLMEngineToolCallTest.h test/AIAgent/LLMEngineToolCallTest.cc \
        test/AIAgent/test_tool_call.cpp test/AIAgent/CMakeLists.txt test/CMakeLists.txt
git commit -m "feat: add tool calling support for Gemma 4 with AgentTool struct and signal emission"
```

**Verification:** `git log --oneline -1` shows the commit

---

## Dependencies & Risks

- **Task 5.3 → 5.4:** SmolLM2 runs first (fast, validates baseline), then Gemma 4 (slow, validates tool calling)
- **Threading model:** `startCompletion()` uses `QMetaObject::invokeMethod(QueuedConnection)`. If LLMEngine is on the main thread (standalone test), this means the lambda is queued to the main thread's event loop. The test's `processEvents()` + `msleep(100)` poll loop should dispatch it. **Risk:** If the queued invocation doesn't execute, we may need to call `loadModel()` also via `invokeMethod` or adjust the event loop. Will discover in Task 5.3.
- **Model determinism:** Gemma 4's tool call is probabilistic. Test is designed to be soft-pass (INFO if no tool call, PASS if detected). We can revisit in future PRs with lower temperature or forced tool_choice.

## Not In Scope

- `parametersJson` field name change (currently `parametersJson`, plan called it `parameterSchema` — the current name is fine, matches OpenAI convention)
- QML UI work
- Model management / settings (PR 6)
- AgentToolBase / Registry (PR 7)