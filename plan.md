# Prepared I16 Formal ABI Publication Runbook

Status: Active
Source Idea: ideas/open/403_prepared_i16_formal_abi_publication.md

## Purpose

Repair the producer-side scalar integer formal ABI publication gap that leaves
direct-call `I16` formals stack-homed without usable prepared ABI facts.

## Goal

Publish prepared `I16` formal ABI/home facts in the same producer repair path
used for adjacent scalar integer widths, then prove the RV64 object route can
consume those facts without reconstructing ABI policy.

## Core Rule

Repair the prepared-BIR producer contract. Do not teach RV64 object emission to
infer missing incoming argument registers or stack homes from parameter order.

## Read First

- `ideas/open/403_prepared_i16_formal_abi_publication.md`
- `todo.md` history from idea 395 commit `e6210244`
- Representative case `tests/c/external/gcc_torture/src/20000403-1.c`
- `src/backend/prealloc/legalize.cpp`
  `direct_bir_call_arg_abi_repair()`
- Prepared dump, if still present:
  `build/agent_state/395_step4_20000403-1.prepared-bir.txt`
- RV64 backend runner `scripts/check_progress_rv64_gcc_c_torture_backend.sh`

## Current Scope

- Inspect scalar formal ABI repair coverage for direct BIR calls.
- Add missing `I16` support at the producer-side prepared ABI publication
  boundary.
- Keep object-emitter parameter-home admission dependent on prepared facts.
- Prove the `src/20000403-1.c` `i16 %p.win` blocker no longer reaches RV64
  object emission as an ABI-less stack-home parameter.

## Non-Goals

- Do not edit RV64 `object_emission.cpp` to reconstruct missing formal ABI
  policy.
- Do not weaken `unsupported_param_home` admission or accept ABI-less scalar
  stack homes.
- Do not redesign aggregate, variadic, byval, sret, or unrelated scalar width
  ABI handling.
- Do not rewrite gcc_torture expectations, supported markers, or allowlists.

## Working Model

- `direct_bir_call_arg_abi_repair()` is the expected producer-side repair
  point because it already handles `I1`, `I8`, `I32`, `I64`, and `Ptr`.
- The observed bad prepared shape is `i16 %p.win` in `seqgt` and `seqgt2`,
  assigned to stack-slot homes backed by regalloc spill slots without usable
  scalar integer ABI facts.
- RV64 object emission should only consume prepared formal ABI/home facts; it
  should not derive missing producer policy from source-level parameter order.

## Execution Rules

- Keep the first packet focused on `I16` direct formal ABI publication.
- Compare the existing scalar width handling before adding a new branch so the
  `I16` behavior follows the established helper contract.
- Preserve diagnostics for truly unsupported or ABI-less parameter homes.
- Record prepared-dump evidence in `todo.md` before and after code changes.
- For each code-changing packet, run the supervisor-delegated build/proof
  command exactly and record results in `todo.md` / `test_after.log`.

## Ordered Steps

### Step 1: Confirm Producer-Side I16 Gap

Goal: verify the current failure is missing producer-side `I16` formal ABI
publication, not an object-emitter lowering gap.

Primary targets:

- `tests/c/external/gcc_torture/src/20000403-1.c`
- `build/agent_state/395_step4_20000403-1.prepared-bir.txt`, if present
- `src/backend/prealloc/legalize.cpp`

Actions:

- Reproduce or load the `src/20000403-1.c` prepared dump and object-route
  diagnostic.
- Confirm `seqgt` and `seqgt2` expose `i16 %p.win` as stack-homed formal
  values without usable scalar integer ABI facts.
- Inspect `direct_bir_call_arg_abi_repair()` and adjacent tests to identify the
  scalar width dispatch used for `I1`, `I8`, `I32`, `I64`, and `Ptr`.
- Record the exact missing-fact evidence and chosen narrow proof command in
  `todo.md`.

Completion check:

- `todo.md` records the current `I16` formal ABI gap, relevant prepared-home
  evidence, and the delegated proof command to use after the repair.

### Step 2: Publish I16 Formal ABI Facts

Goal: extend producer-side direct formal ABI repair so `I16` formals publish
usable prepared scalar integer ABI/home facts.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- Existing prepared-BIR/regalloc tests for direct call/formal ABI repair, if
  present

Actions:

- Add `I16` handling through the same mechanism as adjacent scalar integer
  widths.
- Keep the change producer-side; do not edit RV64 object emission to compensate
  for missing ABI facts.
- Add or adjust narrow tests if there is an existing local prepared-BIR or
  regalloc test surface for direct formal ABI repair.
- Preserve behavior for `I1`, `I8`, `I32`, `I64`, and `Ptr`.

Completion check:

- Prepared dumps or tests show `I16` formals receive usable scalar integer
  ABI/home facts before object emission, without broad unrelated ABI diffs.

### Step 3: Prove 20000403-1.c And Guard Adjacent Widths

Goal: prove the repaired producer facts unblock the representative route and do
not regress adjacent scalar formal widths.

Primary targets:

- `src/20000403-1.c`
- Any narrow prepared-BIR/regalloc tests touched in Step 2
- Canonical `test_after.log` when delegated by the supervisor

Actions:

- Run the exact supervisor-delegated build/proof command.
- Confirm `src/20000403-1.c` no longer fails with the same
  `unsupported_param_home` diagnostic for ABI-less `i16 %p.win`.
- Check whether the case now passes or exposes a later, distinct object-route
  diagnostic; record that result without claiming unrelated bucket completion.
- If tests cover adjacent scalar widths, run the matching narrow subset.
- Record proof logs and remaining blockers in `todo.md`.

Completion check:

- `test_after.log` and `todo.md` show the `I16` producer ABI publication gap is
  repaired or name a distinct later blocker for supervisor routing.

### Step 4: Route Any Remaining Blocker

Goal: return clean lifecycle state to the supervisor once the producer repair is
proved or a different gap is exposed.

Primary targets:

- `todo.md`
- Active source idea `ideas/open/403_prepared_i16_formal_abi_publication.md`
- Related open RV64 object-route ideas if a later target-emission blocker
  appears

Actions:

- If `src/20000403-1.c` passes the producer-home point and exposes a later
  object-route issue, record the new diagnostic in `todo.md`.
- Ask the supervisor to reactivate the relevant object-route idea when the
  producer repair is complete.
- If another producer missing fact appears, split it into a separate source
  idea instead of expanding this one.

Completion check:

- The active lifecycle state clearly says whether this idea is complete,
  blocked by a distinct follow-up, or ready to hand back to object-route work.
