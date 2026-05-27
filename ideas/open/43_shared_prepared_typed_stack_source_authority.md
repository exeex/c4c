# Shared Prepared Typed Stack Source Authority

## Goal

Add target-neutral prepared authority for typed scalar stack-source
edge-publications so consumers such as RISC-V can safely lower sub-word,
unsigned-I32-shaped, and floating stack-source loads without guessing from
size or register spelling.

## Why This Exists

`40_riscv_prepared_edge_publication_typed_stack_source_policy.md` closed as a
validated fail-closed blocker. RISC-V has enough prepared authority for
existing concrete 4-byte `lw`, 8-byte `ld`, and large-offset stack-source
forms, but not for sub-word extension, unsigned-I32-shaped behavior, or F32
stack-source typed loads.

The missing information is semantic: scalar type, signedness/extension policy,
and destination register-bank/view authority. It should not be inferred in
RISC-V from byte size alone, value ids, stack slot ids, offsets, fixture names,
or raw register spelling.

## In Scope

- Audit prepared value-home and edge-publication records for typed scalar
  `StackSlot -> Register` source loads.
- Add target-neutral fields or queries for scalar type,
  signedness/extension policy, and destination register-bank/view when those
  facts are available.
- Preserve fail-closed behavior when typed facts are absent or incomplete.
- Update RISC-V to consume one selected typed scalar stack-source form only
  through shared prepared facts.
- Add focused positive and negative coverage proving the selected form is not
  inferred from size, ids, offsets, fixture names, or register spelling.

## Out Of Scope

- Dynamic-address stack-source support.
- Aggregate-width stack-source support.
- Source-to-`StackSlot` destination broadening.
- Broad RISC-V memory/codegen rewrites.
- Moving target instruction selection or register spelling into shared prepare.

## Acceptance Criteria

- Shared prepared facts can represent at least one typed scalar stack-source
  form without target-local inference.
- RISC-V consumes that fact for one safe lowering path, or records a precise
  missing upstream producer reason if no safe form exists yet.
- Unsupported typed neighboring forms remain explicit and fail closed.
- Existing concrete 4-byte, 8-byte, and large-offset stack-source behavior
  remains supported.
- Focused tests and backend validation pass without expectation weakening.

## Reviewer Reject Signals

- A patch treats byte size alone as signedness, floatingness, or register-bank
  authority.
- A patch adds RISC-V-local producer scans or a target-local edge-copy fact
  table.
- A patch encodes RISC-V register names or load opcodes into shared prepared
  records.
- A patch weakens the fail-closed tests from idea 40 instead of replacing them
  with shared authority.
