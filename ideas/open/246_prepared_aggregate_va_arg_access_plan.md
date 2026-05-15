# Prepared Aggregate `va_arg` Access Plan

Status: Open
Created: 2026-05-15

Parent Context: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Blocked Idea: ideas/open/243_aarch64_variadic_machine_node_consumption.md

## Goal

Add a prepared/shared aggregate `va_arg` access-plan fact that lets AArch64
selected lowering consume aggregate variadic accesses without reconstructing
AAPCS64 source selection, copy extent, aggregate size/alignment, or `va_list`
progression in target codegen.

## Why This Exists

Idea 243 Step 4 reached an exact lifecycle blocker after backend proof passed
in commit `b426f024b`: aggregate `va_arg` has prepared entry storage, helper
scratch, destination payload homes, and source `va_list` homes, but it still
lacks `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`. AArch64
lowering must not derive aggregate source selection, register-save versus
overflow payload coordinates, copy size/alignment, or `va_list` progression
from helper names, local ABI layout rules, or destination-home shape.

## In Scope

- Define a prepared/shared aggregate `va_arg` access-plan record tied to the
  existing variadic helper operand-home carrier.
- Record whether a supported aggregate access uses register-save storage,
  overflow storage, or another explicitly prepared payload source.
- Preserve aggregate payload size, alignment, copy extent, and destination
  payload requirements as structured prepared facts.
- Preserve the source `va_list` home and post-access progression needed after
  the aggregate fetch, including register-offset or overflow-area advancement
  facts where applicable.
- Dump and test the prepared fact independently from selected machine-node
  consumption.
- Keep existing fail-closed diagnostics for incomplete prepared facts and make
  the missing aggregate access-plan fact explicit.

## Out Of Scope

- Do not add AArch64 selected machine-node consumption for aggregate
  `va_arg`; that remains in idea 243 after this prerequisite closes.
- Do not infer AAPCS64 aggregate classification, register-save offsets,
  overflow-area offsets, copy extent, named argument counts, or helper scratch
  policy inside AArch64 target lowering.
- Do not broaden into scalar `va_arg`, `va_start`, `va_copy`, global address,
  memory load/store, scalar cast, i128, binary128, atomic, intrinsic,
  inline-asm, callee-save slot-placement, or preserved-value extent work.
- Do not weaken unsupported expectations or claim machine-node support through
  prepared dump coverage alone.

## Acceptance Criteria

- Prepared variadic helper facts expose
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan` for a
  representative supported aggregate `va_arg` access.
- The fact identifies aggregate source class, payload size/alignment, source
  storage coordinates, destination payload relationship, copy extent, and
  `va_list` progression needed by later selected lowering.
- Missing or incomplete aggregate access-plan facts still fail closed with an
  explicit diagnostic.
- Focused prepared/printer and backend-preparation tests prove the new fact
  without changing selected machine-node support claims.
- Idea 243 can resume Step 4 with the access-plan fact as shared prepared
  authority instead of target-local reconstruction.

## Reviewer Reject Signals

- The route adds named-case matching for one aggregate variadic fixture or
  helper symbol instead of producing a general prepared aggregate access-plan
  record.
- The route computes aggregate source selection, register-save coordinates,
  overflow coordinates, copy extent, `va_list` offsets, named register counts,
  or progression policy in AArch64 target lowering and calls it prepared
  support.
- The route downgrades unsupported expectations, weakens diagnostics, or
  claims selected machine-node consumption before idea 243 resumes.
- The route only renames helper operand-home fields, changes prepared dump
  text, or rewrites expectations while retaining the missing
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan` failure mode.
- The route broadens into scalar variadic access or unrelated backend lowering
  families instead of the aggregate `va_arg` access-plan prerequisite.
