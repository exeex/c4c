# AArch64 Call-Argument Preservation Lookup Checkpoint

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue AArch64 call-emission consolidation after the Step 5 closure review
for the prior-preservation lookup boundary checkpoint rejected source-idea
closure.

## Goal

Remove or narrow the surviving AArch64-local call-argument preservation lookup
boundary that still selects prior preserved values through a non-dominating
indexed lookup and a raw prepared-call-plan fallback.

## Core Rule

Target-local AArch64 calls code may turn prepared call facts into AArch64
machine nodes and assembly spelling. It must not reconstruct call-plan
authority by scanning retained BIR, walking raw prepared-call lists, or
duplicating argument, byval, preservation, dispatch, or printing decisions
that shared prepared data already owns.

## Latest Closure Review Finding

The Step 5 closure review after the prior-preservation lookup boundary
checkpoint rejects source-idea closure.

The checkpoint's broader backend proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
reported 162/162 backend tests passed in the rolled-forward canonical
`test_before.log`.

The source idea remains open because `calls_preservation.cpp` still contains
`find_prior_preserved_value_for_call_argument`, which selects a prior preserved
value with `prepare::find_latest_indexed_prior_preserved_value` and then falls
back to raw prepared-call-plan discovery and iteration when indexed lookups are
unavailable. That surviving lookup boundary is durable remaining preservation
work under the source idea's acceptance criteria.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- Shared prepared-call lookup helpers under `src/backend/prealloc/`
- Focused backend call-boundary and prior-preservation tests under
  `tests/backend/mir/`

## Current Targets / Scope

- `find_prior_preserved_value_for_call_argument`
- `make_prior_preserved_call_argument_source`
- The preservation lookup declarations in `calls.hpp`
- Adjacent `calls_moves.cpp` consumers only where they rely on the selected
  call-argument preservation helper
- Shared prepared-call lookup helpers only if Step 1 proves the fallback exists
  because a required prepared fact is missing

## Non-Goals

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not perform broad dispatch cleanup outside AArch64 call emission.
- Do not invent a new shared prepared-call API unless the selected blocker
  proves a required prepared fact is missing.
- Do not move AArch64-specific register, frame-slot, instruction, or assembly
  spelling details into shared planning.
- Do not weaken unsupported or expected-output contracts.
- Do not change behavior solely to reduce line count.
- Do not absorb argument-source, byval, printing, ALU, memory, comparison,
  variadic, prologue, or unrelated dispatch lowering cleanup into this
  checkpoint.
- Do not revisit completed aggregate-address, local-frame publication,
  prior stack-preservation lookup, block-entry republication, frame-slot call
  argument narrowing, or value-id prior-preservation lookup routes unless this
  checkpoint proves one still owns duplicate authority.

## Working Model

- Treat the non-dominating latest-indexed lookup and raw call-plan fallback in
  `find_prior_preserved_value_for_call_argument` as the selected boundary leak.
- Decide whether the correct behavior is to require dominating prepared lookup
  authority, keep latest-indexed semantics for call arguments, consume an
  existing prepared lookup helper with stricter preconditions, or stop on a
  missing prepared fact.
- Delete or narrow local prepared-call reconstruction before adding replacement
  helper layers.
- Keep target-local code responsible only for turning selected preservation
  facts into AArch64 machine nodes.
- A surviving helper is acceptable only when its parameters and callers describe
  AArch64 emission work rather than prior-call selection.

## Execution Rules

- Start with `calls_preservation.cpp` and preservation-related declarations in
  `calls.hpp`.
- Keep each code slice narrow enough for a fresh build plus focused backend
  proof.
- Escalate to `^backend_` after changing preservation lookup, prepared
  call-plan consumption, move ordering, boundary effects, or call-boundary
  machine instruction emission.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Step 1: Confirm The Call-Argument Lookup Boundary

Goal: identify why `find_prior_preserved_value_for_call_argument` still needs a
non-dominating latest-indexed lookup plus raw prepared-call-plan fallback, and
choose the correct owner for that selection.

Primary targets:

- `find_prior_preserved_value_for_call_argument`
- `make_prior_preserved_call_argument_source`
- `context.function.call_plan_lookups`
- `prepare::find_latest_indexed_prior_preserved_value`
- adjacent `calls_moves.cpp` consumers of
  `make_prior_preserved_call_argument_source`

Actions:

- Map the lookup path to the source idea's acceptance criteria.
- Determine whether all valid lowering contexts should provide
  `call_plan_lookups` for this path.
- Compare the fallback behavior against existing indexed prepared lookup
  helpers and determine whether "latest" or "dominating" lookup is the intended
  semantic contract for call-argument sources.
- Record the selected ownership decision, the intended latest-vs-dominating
  contract, any missing prepared fact, and the focused proof command in
  `todo.md`.

Completion check:

- `todo.md` names the lookup boundary, the intended owner or precise missing
  prepared-fact blocker, the latest-vs-dominating contract, and the focused
  proof scope.

## Step 2: Remove Or Narrow The Lookup Boundary

Goal: eliminate the local raw prepared-call-plan walk and reduce the helper to
the correct prepared-lookup consumer.

Actions:

- Delete the fallback path that discovers `call_plans` and iterates
  `call_plans->calls` locally, unless Step 1 proves the path is required for a
  missing prepared authority.
- Consume existing prepared lookup data when sufficient, using the selected
  latest-vs-dominating contract from Step 1.
- Tighten helper signatures or diagnostics so obsolete context parameters do
  not imply local planning authority.
- Keep source-register and stack-slot machine operand creation in AArch64
  emission code.
- Update focused tests only to preserve or strengthen the prepared-authority
  contract.
- Run `cmake --build --preset default` plus the focused backend proof selected
  in Step 1.

Completion check:

- The selected call-argument preservation helper no longer reconstructs prior
  preserved values from raw prepared call plans, its indexed lookup semantics
  are explicit, and `test_after.log` records a passing build plus focused
  proof.

## Step 3: Consolidate Helper/API Surface

Goal: remove declarations, wrappers, includes, or translation-unit boundaries
made obsolete by Step 2.

Actions:

- Remove obsolete declarations from `calls.hpp`.
- Inline or relocate helper code only when the surviving boundary has a clearer
  emission owner.
- Preserve AArch64-specific operand construction in AArch64 code.
- Run a fresh build and the focused backend proof after each coherent helper
  boundary change.

Completion check:

- The affected preservation helper/API surface is smaller or explicitly
  emission-only, with build metadata and include graphs matching the new
  boundary.

## Step 4: Broader Backend Checkpoint

Goal: prove the latest call-argument preservation lookup checkpoint did not
regress adjacent call emission, prepared-call, preservation, byval, aggregate,
local-frame, dispatch-bridge, or printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 call-boundary, prepared-call, preservation,
  argument-source, byval, aggregate, local-frame/publication, dispatch-bridge,
  and printer tests.
- Include affected shared-boundary and x86 tests if shared prepared-call
  lookup behavior was touched.
- Record exact proof commands and results in `todo.md`.

Completion check:

- The broader checkpoint passes, or `todo.md` records a precise blocker tied to
  the active source idea.

## Step 5: Closure Review

Goal: decide whether the source idea is complete after this checkpoint or
whether another runbook checkpoint is needed.

Actions:

- Recheck all surviving `calls*.cpp` and `calls.hpp` boundaries against the
  source idea acceptance criteria.
- Confirm target-local calls code no longer rederives decisions already present
  in shared prepared facts.
- Confirm surviving helper files are emission-only, printer-owned,
  dispatch-owned outside this source idea, or identified as the next checkpoint
  target.
- If durable remaining work exists, keep the idea open and request another
  runbook checkpoint instead of closing.

Completion check:

- Lifecycle state gives the supervisor an unambiguous close/keep-active
  decision.
