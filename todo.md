Status: Active
Source Idea Path: ideas/open/subsystem-entropy-reduction-refactor-generator.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Create Follow-Up Refactor Ideas

# Current Packet

## Just Finished

Step 4 created three focused follow-up refactor ideas from the ranked AArch64
codegen hotspot list:

1. `ideas/open/aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md`
   for bounded `calls_dispatch_bridge.*` helper absorption.
2. `ideas/open/aarch64-codegen-02-compatibility-projection-bridge-retirement.md`
   for `compatibility_projection.*` bridge retirement, explicitly blocked
   unless selected target records fully own the projected behavior.
3. `ideas/open/aarch64-codegen-03-alu-fallback-operand-phase-extraction.md`
   for optional scalar `alu.cpp` fallback operand phase extraction scoped to
   one helper group.

Each generated idea names target files, one refactor type, a durable owner or
responsibility category, behavior-preservation proof expectations, and reject
signals for expectation downgrades, testcase-shaped shortcuts, hidden semantic
changes, target-specific logic moved into generic layers, giant multi-purpose
refactors, broad renames without durable concept proof, and file-count
reductions that increase responsibility mixing.

## Suggested Next

Step 5 should ask the plan owner to decide whether the umbrella generator plan
is ready to close or should hand off to one generated idea through a normal
lifecycle switch. The safest immediate handoff candidate is
`aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md`; the
compatibility projection idea should remain blocked until selected target
records fully own the projected behavior.

## Watchouts

The new idea files are not activated. Step 5 should not silently expand this
umbrella plan into implementation work; it should close the generator plan or
switch lifecycle state through the plan owner.

The compatibility projection idea has a hard blocker: if selected target
records do not fully own the projected behavior for the selected scope, the
bridge must remain in place.

The ALU idea is intentionally limited to fallback operand selection. If a
future packet needs control-publication materialization or full `alu.cpp`
decomposition, it should create or activate a separate idea.

## Proof

`git diff --check` passed. No build, test run, or `test_after.log` was needed
for this planning-only packet.
