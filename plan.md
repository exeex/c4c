# RV64 Object Route Instruction Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Activated after: ideas/closed/407_prepared_i16_same_module_call_arg_abi_publication.md

## Purpose

Repair the RV64 prepared-object route bucket where prepared BIR instructions
reach object emission but are rejected as unsupported instruction fragments.

## Goal

Classify and lower reusable RV64 instruction-fragment families so the current
representatives advance without testcase-shaped shortcuts or producer-boundary
violations.

## Core Rule

RV64 object emission may lower explicit prepared instruction facts. It must not
reconstruct missing producer ABI facts, BIR control/data-flow semantics, source
testcase intent, or filename-specific behavior.

## Read First

- `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
- `ideas/closed/403_prepared_i16_formal_abi_publication.md`
- `ideas/closed/407_prepared_i16_same_module_call_arg_abi_publication.md`
- `tests/c/external/gcc_torture/src/20000223-1.c`
- `tests/c/external/gcc_torture/src/divmod-1.c`
- Current RV64 gcc_torture backend probe artifacts under `build/agent_state/`
  and `build/rv64_gcc_c_torture_backend/`

## Current Targets

- Primary source-idea representative:
  `src/20000223-1.c`, the original dominant-bucket representative for
  `unsupported_instruction_fragment`.
- Reopened residual representative:
  `src/divmod-1.c`, after closed idea 407 repaired the producer-side
  same-module `i16` call-argument ABI publication gap.
- Nearby same-fragment cases selected by the supervisor from current RV64
  gcc_torture backend artifacts.

## Non-Goals

- Do not reopen closed idea 403 unless fresh evidence shows incoming `i16`
  formal ABI publication regressed.
- Do not reopen closed idea 407 unless fresh prepared dumps show the old
  no-bank same-module `i16` call-argument shape again.
- Do not infer scalar call-argument registers, destination banks, parameter
  homes, or ABI policy in `src/backend/riscv/rv64/object_emission.cpp`.
- Do not absorb terminator lowering, move-bundle target shapes, stack-frame
  admission, parameter-home, global data, or runtime-mismatch work owned by
  separate open ideas.
- Do not rewrite gcc_torture expectations, mark cases unsupported, weaken
  allowlists, or special-case testcase filenames.

## Working Model

Previous 395 execution split producer-side ABI gaps to 403 and 407. Both are
closed. `src/divmod-1.c` now publishes the same-module `i16` call-argument GPR
destination facts that 407 owned, but still stops at generic
`unsupported_instruction_fragment`. The next work must reclassify the prepared
instruction shape now visible at the object-emission boundary and decide
whether it is a true RV64 instruction-lowering gap or another distinct owner.

## Execution Rules

- Keep each packet tied to one concrete prepared instruction-fragment family.
- Inspect prepared dumps and object-route diagnostics before editing target
  emission.
- Prefer semantic RV64 lowering that applies to a same-fragment family.
- If a representative lacks required prepared facts, stop and route that
  producer boundary instead of compensating in the RV64 emitter.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- If a repaired case object-compiles and links, include qemu comparison in the
  proof.
- Treat diagnostic-only churn, expectation rewrites, allowlist-only progress,
  and named-case green proof as route failures.

## Step 1: Reclassify Current Instruction-Fragment Residuals

Goal: identify the concrete prepared instruction fragments currently blocking
395 after the closed producer-side splits.

Actions:

- Reproduce or inspect the supervisor-selected RV64 gcc_torture backend probe
  for `src/divmod-1.c`, `src/20000223-1.c`, and any nearby same-fragment cases.
- Dump or inspect prepared BIR for each representative and name the exact
  instruction opcode, operand facts, value banks, placements, and source/dest
  facts that reach object emission.
- Map each rejection to the RV64 object-route code that emits
  `unsupported_instruction_fragment`.
- Decide whether `src/divmod-1.c` and `src/20000223-1.c` share a reusable
  lowering route or require separate packets.
- Check for evidence that a residual still belongs to a closed producer split
  or another open owner before assigning it to 395 implementation.

Completion check:

- `todo.md` records the concrete instruction-fragment family for the first
  executor packet, the representative set, and the exact supervisor-delegated
  proof command.
- Any non-395 residual is routed with precise evidence instead of patched in
  RV64 object emission.

## Step 2: Lower The First Valid Instruction-Fragment Family

Goal: add reusable RV64 object lowering for the first classified prepared
instruction-fragment family when all required prepared facts are explicit.

Actions:

- Update the RV64 object route to consume the prepared instruction facts for
  the selected family.
- Preserve existing diagnostics for unsupported or incomplete prepared facts.
- Add or update focused backend coverage where the repo has a matching test
  surface for the instruction family.
- Keep changes narrow to the selected instruction semantics and existing
  prepared-object contracts.

Completion check:

- The selected representative no longer fails with the same
  `unsupported_instruction_fragment` diagnostic.
- Nearby same-fragment cases examined by the executor either advance together
  or are explicitly classified as separate owners.
- Existing backend tests for adjacent instruction lowering remain green.

## Step 3: Prove Representatives And Bucket Movement

Goal: prove the lowering advanced 395 without hiding runtime or ownership
failures.

Actions:

- Run the supervisor-selected narrow RV64 gcc_torture backend probe for the
  repaired representative and same-fragment additions.
- If a case object-compiles and links, inspect qemu comparison rather than
  stopping at compile success.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any remaining failure as the same instruction-fragment family, a
  distinct target-emission family, or a producer-fact gap that needs a separate
  source idea.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, unsupported downgrades, allowlist filtering, or
  filename-specific fixes are used as acceptance evidence.
- The supervisor has enough evidence to continue with another 395 packet,
  request route review, or ask the plan owner for close/deactivation handling.
