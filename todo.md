Status: Active
Source Idea Path: ideas/open/subsystem-entropy-reduction-refactor-generator.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Lifecycle Handoff Or Close Decision

# Current Packet

## Just Finished

Step 5 reviewed the active source idea acceptance criteria, `plan.md`,
current `todo.md`, the committed Step 2 and Step 3 `todo.md` evidence, and the
three generated AArch64 codegen follow-up ideas.

Acceptance is satisfied for the umbrella generator run:

- Step 2 produced the required entropy map for
  `src/backend/mir/aarch64/codegen/`, including concrete hotspots in
  `instruction.*`, `dispatch.cpp`, `alu.*`, `calls_moves.cpp` /
  `calls.hpp`, `calls_dispatch_bridge.*`, `compatibility_projection.*`,
  printer/emitter files, and cross-cutting text-emission leakage.
- Step 3 produced the required prioritized hotspot list ordered by safety,
  reuse value, and durable ownership clarity.
- Step 4 produced multiple actionable follow-up ideas under `ideas/open/`,
  ordered by safety and reuse value:
  `aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md`,
  `aarch64-codegen-02-compatibility-projection-bridge-retirement.md`, and
  `aarch64-codegen-03-alu-fallback-operand-phase-extraction.md`.
- Each generated idea names target files, one refactor type, a durable owner
  or responsibility category, expected behavior-preservation proof, and reject
  signals.
- The umbrella run did not require implementation, test, expectation, or log
  edits.

Decision: recommend plan-owner closure of the umbrella generator plan. The
source idea's acceptance is met, so the active runbook should not silently
expand into implementation work.

## Suggested Next

Ask the plan owner to close
`ideas/open/subsystem-entropy-reduction-refactor-generator.md` through normal
lifecycle rules. After closure, the supervisor may activate one generated idea
as a separate lifecycle state; the safest first candidate remains
`ideas/open/aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md`.

## Watchouts

The generated ideas are not activated and should stay separate from this
umbrella plan's closure.

The compatibility projection idea has a hard blocker: if selected target
records do not fully own the projected behavior for the selected scope, the
bridge must remain in place.

The ALU idea is intentionally limited to fallback operand selection. If a
future packet needs control-publication materialization or full `alu.cpp`
decomposition, it should create or activate a separate idea.

## Proof

`git diff --check` passed. No build, test run, or `test_after.log` was needed
for this planning-only packet.
