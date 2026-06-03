# AAPCS64 Va Arg Payload Shape Authority Runbook

Status: Active
Source Idea: ideas/open/102_aapcs64_va_arg_payload_shape_authority.md

## Purpose

Make AAPCS64 `va_arg` payload ABI and aggregate/HFA shape authority explicit so
prealloc does not silently infer semantic payload shape from lowered load names,
slot suffixes, or memory access patterns.

## Goal

Define and implement the narrow authority boundary for scalar, aggregate, and
HFA AAPCS64 `va_arg` payload planning.

## Core Rule

Do not add new slot-name parsing, testcase-shaped matching, or broad AArch64
call ABI rewrites. Either publish the needed payload facts from BIR/runtime
placeholder lowering, or document and constrain a narrow AAPCS64 helper-local
reconstruction contract for the intended `va_arg` placeholder route.

## Read First

- `ideas/open/102_aapcs64_va_arg_payload_shape_authority.md`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/variadic.hpp`
- BIR runtime placeholder call definitions and prepared-call dump surfaces
- Existing AAPCS64 variadic tests under `tests/backend/`

## Current Targets

- Scalar AAPCS64 `va_arg` payload ABI authority.
- Aggregate/HFA AAPCS64 `va_arg` lane count, lane size, and destination-home
  semantics.
- The route through runtime `va_arg` placeholder calls into variadic entry
  planning.
- Focused proof for scalar, aggregate, and HFA `va_arg` behavior.

## Non-Goals

- Do not rework fixed-argument AArch64 HFA pressure.
- Do not change non-AAPCS64 variadic entry behavior unless a shared interface
  makes a small compatibility adjustment unavoidable.
- Do not generalize all aggregate call ABI facts beyond the `va_arg` payload
  shape boundary.
- Do not weaken existing variadic tests or mark supported paths unsupported.

## Working Model

BIR already carries fixed-call HFA lane facts, but variadic aggregate payload
shape currently has no explicit target-neutral or documented helper-local
contract. Prealloc may still own AAPCS64 helper planning and physical placement,
but it must not become silent semantic authority by deriving payload shape from
post-call loads, aggregate slot-name suffixes, or incidental frame layout.

## Execution Rules

- Keep analysis findings in `todo.md` unless they change the active runbook.
- Prefer named helpers or metadata fields over anonymous reconstruction.
- If BIR owns a fact, make the producer and consumer visible in tests or dumps.
- If the helper-local route owns a reconstruction, name the predicate and limit
  it to the runtime `va_arg` placeholder route.
- For code-changing steps, run `cmake --build --preset default` before handing
  the packet back.
- Escalate validation beyond the narrow subset if shared ABI or variadic
  interfaces change.

## Step 1: Inventory Existing Va Arg Shape Authority

Goal: identify all current producers and consumers for scalar, aggregate, and
HFA AAPCS64 `va_arg` payload facts.

Primary targets:

- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/variadic.hpp`
- BIR runtime placeholder call construction
- Existing AAPCS64 variadic tests

Actions:

- Trace scalar `va_arg` planning from placeholder call `return_type` to payload
  ABI selection.
- Trace aggregate and HFA `va_arg` planning through
  `infer_aapcs64_hfa_va_arg_shape` and related destination-home logic.
- List every fact that comes from BIR metadata, every fact reconstructed in
  prealloc, and every fact inferred from names, loads, slots, or frame layout.
- Record candidate authority gaps in `todo.md`; do not edit source intent.

Completion check:

- `todo.md` names the concrete producer and consumer surfaces and separates
  legitimate prealloc placement decisions from semantic payload-shape facts.
- Analysis proof confirms no implementation diff under `src/backend/bir` or
  `src/backend/prealloc`.

## Step 2: Decide The Narrow Authority Contract

Goal: choose whether payload ABI and HFA/aggregate shape facts are explicit BIR
placeholder metadata or a constrained AAPCS64 helper-local contract.

Actions:

- Compare the inventory against the source idea acceptance criteria.
- Decide which facts must be carried explicitly and which, if any, may be
  reconstructed locally.
- Name the contract surface and reject any route that depends on additional
  slot-name or testcase-shaped parsing.
- Identify focused proof cases for scalar, aggregate, and HFA `va_arg`.

Completion check:

- `todo.md` records a concrete contract decision, implementation targets, and
  proof targets.
- Analysis proof confirms no implementation diff unless the contract decision
  packet intentionally includes the first implementation slice.

## Step 3: Implement The Contract

Goal: make variadic entry planning consume the chosen authority instead of
silent payload-shape inference.

Actions:

- Add or expose the selected BIR metadata/helper-local contract.
- Update scalar `va_arg` planning to use the named authority.
- Update aggregate/HFA `va_arg` planning to consume explicit lane count, lane
  size, and destination-home facts or the documented narrow reconstruction.
- Remove, isolate, or assert against old name/load/slot-derived authority where
  the new contract supersedes it.
- Preserve target ABI behavior for already-supported cases.

Completion check:

- The default build passes.
- The diff does not add new slot-name parsing or narrow testcase matching.
- Remaining reconstruction paths are named, documented in code only where
  useful, and restricted to the intended placeholder route.

## Step 4: Add Focused Proof

Goal: prove the authority boundary for scalar, aggregate, and HFA AAPCS64
`va_arg` cases.

Actions:

- Add or strengthen focused backend tests for scalar `va_arg` payload ABI.
- Add or strengthen aggregate `va_arg` tests covering destination-home
  behavior.
- Add or strengthen HFA aggregate `va_arg` tests covering lane count and lane
  size.
- Prefer assertions on the contract surface or prepared plans over fragile
  assembly-only symptoms when possible.

Completion check:

- The default build passes.
- Focused backend tests covering AAPCS64 variadic planning pass.
- No expectations are weakened or marked unsupported.

## Step 5: Final Validation And Close Readiness

Goal: prove the completed route and prepare the source idea for closure review.

Actions:

- Run the default build and the relevant AAPCS64 variadic/backend subset.
- Include adjacent call-boundary or prepared-plan tests if shared interfaces
  changed.
- Update `todo.md` with final proof, retained exceptions if any, and close
  readiness notes.

Completion check:

- Final delegated proof passes.
- `todo.md` shows scalar, aggregate, and HFA `va_arg` coverage.
- The source idea acceptance criteria are satisfied without broad unrelated ABI
  rewrites.
