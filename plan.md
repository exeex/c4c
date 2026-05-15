# AArch64 Binary128 Softfloat Lowering Follow-On Runbook

Status: Active
Source Idea: ideas/open/237_aarch64_binary128_softfloat_lowering.md

## Purpose

Resume the binary128 AArch64 parent route now that the full-width F128
constant-carrier dependency has closed. This runbook is a follow-on checkpoint,
not a restart of the earlier binary128 work: prior route proof already covered
full-width F128 transport, helper boundaries, selected machine records,
printer output, and representative backend flow.

## Goal

Integrate structured full-width F128 constant payload authority into the
binary128 parent route, prove that constants participate in the supported
AArch64 backend path without scalar shortcuts, then decide whether the source
idea can close.

## Core Rule

Every `F128` value remains a full 16-byte value across prepared facts, constant
payload carriers, helper boundaries, storage, reload, selected machine records,
and printing. Do not claim progress through `F64`, `double`, single-lane
immediates, rendered text reconstruction, fixed scratch snippets, or
testcase-shaped helper shortcuts.

## Read First

- `ideas/open/237_aarch64_binary128_softfloat_lowering.md`
- `ideas/closed/241_f128_full_width_constant_carriers.md`
- `docs/backend/x86_codegen_legacy/f128.cpp.md`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`

## Current Targets

- Parent-route use of the structured full-width F128 constant carrier landed
  by the closed dependency.
- AArch64 selected records and diagnostics that distinguish valid full-width
  F128 constants from missing, partial, or scalar-only payloads.
- Representative binary128 backend proof covering constant input, helper or
  transport use, and full-width result preservation.
- Closure judgment for the parent source idea after adjacent unsupported
  helper, atomic, intrinsic, and inline-assembly routes remain out of scope.

## Non-Goals

- Do not reopen the closed F128 constant-carrier design unless a defect proves
  the dependency incomplete.
- Do not assume hardware quad-FP arithmetic on AArch64.
- Do not route binary128 arithmetic, comparisons, casts, constants, or
  sign-bit operations through scalar `F32` or `F64` records.
- Do not add atomic, intrinsic, inline-assembly, scalar FP, or i128 behavior to
  this parent route.
- Do not weaken unsupported expectations or external testcase contracts to
  claim binary128 progress.
- Do not add final assembly printing for a binary128 form unless selected
  structured records already own every operand and payload fact.

## Working Model

- Treat closed idea 241 as the authority for exact F128 constant payload
  transport through BIR, prepared state, and AArch64 selection.
- Treat the earlier completed binary128 runbook as the authority for
  non-constant full-width transport, helper boundaries, selected records, and
  printer proof.
- Prefer one or two focused executor packets: first prove integration gaps, then
  close the parent route if the source idea is truly satisfied.
- Keep unsupported helper families fail-closed when their prepared helper or
  printer authority is still absent.

## Execution Rules

- Each code-changing step must run the supervisor-delegated build or compile
  proof and the matching backend test subset.
- Add tests beside the backend surface being changed; prefer record-level tests
  before final assembly text tests.
- Preserve scalar FP, i128, and already-closed F128 constant-carrier behavior
  after every shared BIR, prepared, dispatch, or printer touch.
- If execution discovers a new prerequisite broader than binary128 parent-route
  integration, stop and ask the supervisor to create or activate a separate
  idea instead of expanding this runbook.

## Ordered Steps

### Step 1: Reconcile Parent Binary128 Route With Constant Carrier

Goal: identify and close any remaining gap between the parent binary128 route
and the full-width F128 constant-carrier dependency.

Primary targets:
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- existing F128 prepared and dispatch tests

Actions:
- Inspect all parent-route F128 constant, helper, transport, and reload paths
  against the structured carrier facts from closed idea 241.
- Add or adjust only the minimal backend integration needed for a semantic F128
  constant to enter the existing full-width binary128 route.
- Keep missing, partial, or scalar-only F128 constants diagnosed rather than
  reconstructed target-locally.
- Confirm no already-supported helper, comparison, cast, sign-bit, or
  load/store path regresses while constants join the route.

Completion check:
- Focused backend tests prove valid full-width F128 constants can reach an
  existing binary128 backend consumer.
- Tests still reject missing, partial, or scalar-only F128 constant payloads.
- `todo.md` records the executor proof command and any remaining parent-route
  closure concerns.

### Step 2: Prove Representative Binary128 Parent Route Closure

Goal: show the source idea's proof direction is satisfied after constant
carrier integration.

Primary targets:
- backend route tests under `tests/backend`
- AArch64 prepared, dispatch, and printer tests chosen by the supervisor

Actions:
- Add or update a representative route proof covering F128 constant or loaded
  input, soft-float helper or full-width transport, and full-width store-back
  or return.
- Run the supervisor-selected narrow proof and a broader backend validation if
  shared BIR, prepared, dispatch, or printer surfaces changed.
- Verify the diff contains no unsupported expectation downgrade, named-case
  matching, scalar `F64` approximation, or fixed scratch-register workaround.
- Record remaining unsupported operations only as explicit out-of-scope or
  future-route notes, not hidden parent-route failures.

Completion check:
- Representative binary128 backend proof is green.
- Broader backend proof is green when required by the supervisor.
- The source idea is ready for plan-owner closure review if no parent-route
  acceptance criterion remains unmet.

### Step 3: Close Or Deactivate The Parent Idea

Goal: make the lifecycle decision explicit after Step 2 proof.

Primary targets:
- `ideas/open/237_aarch64_binary128_softfloat_lowering.md`
- `plan.md`
- `todo.md`

Actions:
- Compare the completed proof against the source idea's scope and proof
  direction, not just this runbook.
- Close the source idea only if full-width memory transport, helper calls,
  comparisons or casts, constants, diagnostics, and representative backend
  proof are all satisfied.
- If a durable unsupported binary128 family remains within the source idea,
  deactivate this runbook with a compact leftover note instead of closing the
  source idea.

Completion check:
- The plan owner either closes the source idea with regression guard proof or
  leaves a clear open-idea note explaining the remaining parent-route work.
