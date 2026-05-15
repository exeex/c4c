# Prepared Scalar `va_arg` Access Plan

Status: Open
Created: 2026-05-15

Parent Context: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Blocked Idea: ideas/open/243_aarch64_variadic_machine_node_consumption.md

## Goal

Add a prepared/shared scalar `va_arg` access-plan fact that lets AArch64
selected lowering consume scalar variadic accesses without reconstructing
AAPCS64 source selection, value size/alignment, or `va_list` progression in
target codegen.

## Why This Exists

Idea 243 Step 3 reached an exact lifecycle blocker after backend proof passed
in commit `a9b41b2a2`: scalar `va_arg` has prepared entry storage, helper
scratch, source `va_list` homes, and scalar result homes, but it still lacks
`helper_operand_homes.va_arg.scalar_access_plan`. AArch64 lowering must not
derive GP/FP register-save versus overflow source, per-access size/alignment,
or `va_list` progression from helper names, result types, named register
counts, or local ABI layout rules.

## In Scope

- Define a prepared/shared scalar `va_arg` access-plan record tied to the
  existing variadic helper operand-home carrier.
- Record whether a supported scalar access uses GP register-save, FP
  register-save, or overflow storage.
- Preserve per-access value size, alignment, and destination/result
  requirements as structured prepared facts.
- Preserve the `va_list` progression needed after the scalar fetch, including
  register offset or overflow-area advancement facts.
- Dump and test the prepared fact independently from selected machine-node
  consumption.
- Keep existing fail-closed diagnostics for incomplete prepared facts and make
  the missing scalar access-plan fact explicit.

## Out Of Scope

- Do not add AArch64 selected machine-node consumption for scalar `va_arg`;
  that remains in idea 243 after this prerequisite closes.
- Do not infer AAPCS64 `va_list` layout, register-save offsets, overflow-area
  offsets, named GP/FP counts, or helper scratch policy inside AArch64 target
  lowering.
- Do not broaden into aggregate `va_arg`, `va_start`, `va_copy`, global
  address, memory load/store, scalar cast, i128, binary128, atomic, intrinsic,
  inline-asm, callee-save slot-placement, or preserved-value extent work.
- Do not weaken unsupported expectations or claim machine-node support through
  prepared dump coverage alone.

## Acceptance Criteria

- Prepared variadic helper facts expose `helper_operand_homes.va_arg.scalar_access_plan`
  for representative supported scalar `va_arg` accesses.
- The fact identifies the scalar source class, size/alignment requirements,
  source storage coordinates, destination/result home relationship, and
  `va_list` progression needed by later selected lowering.
- Missing or incomplete scalar access-plan facts still fail closed with an
  explicit diagnostic.
- Focused prepared/printer and backend-preparation tests prove the new fact
  without changing selected machine-node support claims.
- Idea 243 can resume Step 3 with the access-plan fact as shared prepared
  authority instead of target-local reconstruction.

## Reviewer Reject Signals

- The route adds named-case matching for one variadic fixture or helper symbol
  instead of producing a general prepared scalar access-plan record.
- The route computes GP/FP register-save selection, overflow selection,
  `va_list` offsets, named register counts, or progression policy in AArch64
  target lowering and calls it prepared support.
- The route downgrades unsupported expectations, weakens diagnostics, or
  claims selected machine-node consumption before idea 243 resumes.
- The route only renames helper operand-home fields, changes prepared dump
  text, or rewrites expectations while retaining the missing
  `helper_operand_homes.va_arg.scalar_access_plan` failure mode.
- The route broadens into aggregate variadic access or unrelated backend
  lowering families instead of the scalar `va_arg` access-plan prerequisite.
