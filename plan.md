# AArch64 Global Materialization Consumer Authority Runbook

Status: Active
Source Idea: ideas/open/54_aarch64_global_value_materialization_consumer_authority_repair.md

## Purpose

Repair the remaining non-dispatch AArch64 global-load materialization consumers
so they consume shared prepared global-address policy instead of recovering
global symbol identity or GOT/direct policy locally.

## Goal

Make `globals.cpp` and `fp_value_materialization.cpp` use the shared prepared
global-address authority established by the dispatch materialization repair,
adding only a narrow shared query if these consumers need one.

## Core Rule

Authority must come from prepared global-address policy, not from local
global-name spelling, `@`-prefix checks, module-global searches, or consumer
local GOT/direct policy inference.

## Read First

- `ideas/open/54_aarch64_global_value_materialization_consumer_authority_repair.md`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`
- Existing prepared global-address policy definitions and lookup helpers added
  for dispatch value materialization.

## Current Targets

- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`
- Shared prepared global-address policy/query surfaces only when the existing
  contract cannot express these consumers' needs.

## Non-Goals

- Do not edit `dispatch_value_materialization.cpp` except for mechanical
  compatibility with a shared query required by these consumers.
- Do not change AArch64 instruction spelling, relocation mechanics, or
  GOT/direct emission behavior except to consume a shared semantic contract.
- Do not fold memory, ALU, calls, comparison, publication, or dispatch
  select-chain repairs into this plan.
- Do not weaken supported-path expectations or mark existing supported global
  materialization routes unsupported.

## Working Model

- `globals.cpp` and `fp_value_materialization.cpp` may still be legitimate
  consumers of global-load materialization behavior.
- Their semantic authority for global symbol identity and GOT/direct policy
  should be prepared before lowering reaches consumer code.
- If an existing prepared global-address policy lookup already answers the
  consumer question, use it directly.
- If the existing lookup is dispatch-shaped, add a narrow shared query rather
  than recreating local recovery inside the consumer.

## Execution Rules

- Keep each code-changing step behavior-preserving apart from authority-source
  repair.
- Prefer deleting local recovery after the shared prepared fact is consumed;
  avoid retaining the old path behind a renamed helper.
- Do not add named-case shortcuts around a single testcase.
- For each implementation step, run at least a fresh build or compile proof and
  the narrow tests selected by the supervisor.
- Escalate to broader backend validation before treating the idea as complete,
  because top-level global-load lowering can affect multiple routes.

## Step 1: Audit Existing Consumer Recovery

Goal: identify the exact local recovery paths in the two owned consumers and
the existing prepared global-address policy they should consume.

Primary targets:

- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`
- Prepared global-address policy definitions and lookup helpers.

Actions:

- Inspect `make_load_global_got_materialization_instruction` and nearby
  global-load materialization helpers in `globals.cpp`.
- Inspect FP producer `LoadGlobal` materialization in
  `fp_value_materialization.cpp`.
- Trace the dispatch materialization prepared-address policy to determine
  whether the current query can serve non-dispatch consumers.
- Record in `todo.md` which consumer paths still recover global symbol or
  GOT/direct policy locally and which prepared fact should replace each one.

Completion check:

- The current consumer recovery sites and the needed prepared authority source
  are named precisely enough for a bounded implementation packet.

## Step 2: Route `globals.cpp` Through Prepared Authority

Goal: make ordinary global-load materialization consume prepared
global-address policy instead of local name or policy recovery.

Primary target:

- `src/backend/mir/aarch64/codegen/globals.cpp`

Actions:

- Replace local global-symbol or GOT/direct policy recovery in the global-load
  materialization path with the existing shared prepared query when possible.
- If no existing query fits, add the smallest shared lookup needed for this
  consumer and route `globals.cpp` through it.
- Remove or narrow obsolete fallback code so the old recovery is not preserved
  as durable authority.
- Keep emitted instruction spelling and relocation behavior unchanged.

Completion check:

- Ordinary global-load materialization obtains symbol and GOT/direct policy
  from prepared authority, with narrow proof covering the affected route.

## Step 3: Route FP `LoadGlobal` Materialization Through Prepared Authority

Goal: make FP global-load value materialization consume the same shared
prepared global-address authority.

Primary target:

- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`

Actions:

- Replace local symbol or GOT/direct policy recovery for FP `LoadGlobal`
  producers with the shared prepared query used by the ordinary route.
- Extend the shared query only if the FP route needs a consumer-specific field
  that is still semantic rather than print-local.
- Remove or narrow obsolete fallback code without changing FP instruction
  spelling or register behavior.

Completion check:

- FP `LoadGlobal` materialization obtains global-address identity and policy
  from prepared authority, with narrow proof covering the affected route.

## Step 4: Consolidate Shared Query Shape

Goal: ensure any new shared global-address query is narrow, reusable by both
non-dispatch consumers, and compatible with the dispatch materialization
surface.

Primary targets:

- Shared prepared global-address policy structures and lookup helpers.
- Any mechanical call-site compatibility in dispatch value materialization, if
  required by the query shape.

Actions:

- Verify the query contract is keyed by prepared semantic identity, not text
  spelling.
- Keep dispatch compatibility mechanical; do not reopen dispatch-specific
  route work under this idea.
- Update or add focused tests only when needed to prove the shared contract is
  exercised by the global-load and FP materialization routes.

Completion check:

- Both non-dispatch consumers share the same prepared authority surface, and
  any dispatch call-site adjustment is mechanical and behavior-preserving.

## Step 5: Final Validation And Closure Check

Goal: prove the idea's source acceptance criteria are satisfied without
testcase overfit or expectation downgrades.

Actions:

- Run the supervisor-selected narrow global-load materialization proof.
- Run the supervisor-selected FP `LoadGlobal` materialization proof.
- Run broader backend validation sufficient to catch top-level global-load
  lowering regressions.
- Check the diff for named-case shortcuts, hard-coded global spelling,
  unsupported-test rewrites, helper-only renames, or broad unrelated rewrites.

Completion check:

- All acceptance criteria in the source idea are satisfied and the remaining
  work, if any, is outside this source idea's scope.
