# Prepared Return Destination Home Authority Runbook

Status: Active
Source Idea: ideas/open/629_prepared_return_destination_home_authority.md

## Purpose

Publish explicit prepared destination-home authority for function return values
so RV64 return move-bundle lowering can consume concrete homes without
inferring return transfers from ABI convention, final assembly shape, or source
case behavior.

## Goal

Make at least one representative return move-bundle row expose explicit
prepared destination-home authority, or reclassify the row to a precise
non-return-authority owner with current diagnostics.

## Core Rule

Return transfers must be driven by prepared function-return facts. RV64 object
emission must fail closed when return source, destination home, width, or
transfer authority is missing, ambiguous, or only derivable from ABI
convention.

## Read First

- `ideas/open/629_prepared_return_destination_home_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/target_abi_contract_research/04_current_prepared_value_consumption_model.md`
- Prepared value-location and move-bundle definitions in
  `src/backend/prealloc/value_locations.hpp` and related regalloc call/return
  planning files.
- RV64 return move-bundle consumers in
  `src/backend/mir/riscv/codegen/object_emission.cpp`, especially
  `return_stack_to_register`, `BeforeReturn`, and `FunctionReturnAbi` paths.

## Current Targets

- Representative return rows named by the source idea:
  - `src/20001130-2.c`
  - `src/20080719-1.c`
- Prepared return facts for scalar source value identity, source home,
  destination home, ABI return register or stack home, width, and function
  return association.
- RV64 object-emission guards for before-return move bundles and
  `return_stack_to_register` shapes.

## Non-Goals

- Ordinary scalar call argument/result transport already handled by idea 613.
- Pointer stack-result policy from idea 627.
- FPR ABI/frame policy from idea 628.
- Generic move-bundle authority unrelated to function returns.
- Runtime mismatch triage, local/global producer repair, variadic/library
  policy, expectation changes, unsupported marker changes, allowlists,
  timeouts, or accounting.

## Working Model

- Evidence comes first. Refresh the named rows before choosing producer or
  consumer edits.
- Destination-home authority belongs to prepared return production when the
  transfer is a function-return transfer.
- Existing prepared carriers should be reused when they can express the needed
  return source, destination, width, and association facts.
- Consumer admission should be narrow and fact-driven. It may unblock a row
  only when explicit prepared return authority exists.
- Reclassify rows that prove to be non-return-authority gaps instead of
  broadening this idea.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit the source idea unless durable intent changes or closure notes
  are required.
- Add focused prepared-layer or RV64 object-emission tests for any code-changing
  step.
- Use `cmake --build --preset default` plus a supervisor-selected backend
  subset as the normal proof ladder for code slices.
- Treat expectation rewrites, unsupported-marker changes, testcase-shaped
  source matching, and final-assembly inference as route failures.

## Step 1: Refresh Return Authority Evidence

Goal: identify the current return destination-home blockers for the
representative rows.

Actions:
- Re-run focused probes for `src/20001130-2.c`, `src/20080719-1.c`, and any
  immediately adjacent return-transfer rows the diagnostics reveal.
- Capture prepared dumps, RV64 object-route diagnostics, and current failure
  bucket labels.
- Distinguish prepared return destination-home gaps from scalar call/result,
  pointer stack-result, FPR, generic move-bundle, runtime, local/global, and
  variadic/library gaps.
- Record evidence and the next owner bucket in `todo.md`.

Completion check:
- `todo.md` lists the rows inspected, the current prepared return facts, the
  RV64 rejection or emission point, and a recommended next step that is not
  named-case-only.

## Step 2: Trace Return Producer Authority

Goal: locate the producer and carrier boundary that should publish explicit
return destination homes.

Actions:
- Trace prepared return planning and before-return move-bundle creation for
  the Step 1 in-scope bucket.
- Identify the carrier fields that already hold, or should hold, source home,
  destination home, width, return ABI storage, and function association.
- Locate the first missing authority boundary before RV64 object emission.
- Record fail-closed conditions for missing, ambiguous, mismatched, or
  unsupported return authority.

Completion check:
- `todo.md` names the exact producer functions, carrier fields, consumer
  checks, and the smallest code-changing packet that can publish or consume
  explicit return destination-home authority.

## Step 3: Publish Or Verify Prepared Return Destination Homes

Goal: ensure prepared-layer facts are explicit for one in-scope scalar return
transfer family before RV64 lowering depends on them.

Actions:
- If upstream authority exists but is not published, add the narrow producer
  publication path.
- If publication already exists, add focused coverage proving the facts are
  complete before object emission.
- Preserve source value identity, source home, destination home, width, return
  ABI storage, and function association.
- Do not infer facts from source filenames, ABI register convention, final
  assembly shape, or return instruction spelling.

Completion check:
- Focused backend coverage proves the selected return destination-home facts
  are present in the prepared representation, or `todo.md` reclassifies the
  bucket with precise missing upstream authority.

## Step 4: Add Narrow RV64 Return Consumer Admission

Goal: allow RV64 object emission to consume only explicit prepared return
destination-home facts for the selected family.

Actions:
- Replace any broad rejection or implicit return-home inference for the
  selected family with validation of prepared return source and destination
  facts.
- Emit only when source home, destination home, width, move phase, destination
  kind, destination storage, and return association match the consumer
  contract.
- Add fail-closed tests for absent, malformed, mismatched, ambiguous, or
  unsupported return authority.
- Preserve existing scalar call/result, pointer stack-result, FPR, and
  non-return move-bundle behavior.

Completion check:
- Focused positive and negative backend tests pass, and the RV64 path rejects
  return transfers without explicit prepared destination-home authority.

## Step 5: Reclassify Representative Rows

Goal: determine whether the source idea is complete, needs another return
authority packet, or should split remaining work into separate initiatives.

Actions:
- Re-run the representative row probes after any Step 3 or Step 4 changes.
- Classify each remaining failure into return destination-home authority or an
  out-of-scope owner bucket.
- Record whether idea 629 is close-ready or which next return authority packet
  is justified.

Completion check:
- `todo.md` contains row-by-row classification, proof results, and a clear
  close/split/continue recommendation for the supervisor.
