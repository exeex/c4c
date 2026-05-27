# Shared Prepared Aggregate Stack Source Authority

## Goal

Define shared prepared authority for aggregate-width stack-source
edge-publications, including copy width, destination/lane mapping, alignment,
partial-copy policy, ABI layout, and scratch ownership.

## Why This Exists

`42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md` closed as
a validated fail-closed blocker. Aggregate-width `StackSlot -> Register`
sources cannot be safely treated as scalar loads based on byte size alone.
They need explicit copy and layout authority before RISC-V, x86, or AArch64
can share the semantic decision while keeping target-specific emission local.

Without shared facts, each target must either remain fail-closed or risk
inventing aggregate lane/source semantics locally.

## In Scope

- Audit aggregate stack-source edge-publication facts currently available from
  shared prepare.
- Define the minimal target-neutral authority for one aggregate source family:
  copy width, destination/lane mapping, alignment, partial-copy behavior, ABI
  layout reference, and scratch ownership expectations.
- Keep target instruction selection and register spelling local.
- Update one target consumer only if a safe narrow aggregate form has complete
  shared facts.
- Add focused tests proving aggregate stack sources are not lowered as scalar
  loads without explicit aggregate authority.

## Out Of Scope

- Typed scalar stack-source support.
- Dynamic-address stack-source support.
- General aggregate ABI redesign.
- Broad source-to-stack destination broadening.
- Treating all aggregate sizes as one scalar integer load.

## Acceptance Criteria

- One aggregate stack-source authority path is represented in shared prepared
  facts, or the close note records the exact upstream facts still missing.
- Existing scalar stack-source behavior remains supported.
- Unsupported aggregate widths, lane mappings, partial copies, alignment
  cases, and scratch requirements remain explicit and fail closed.
- No target consumes aggregate stack sources by byte-size-only scalar overfit.
- Focused proof and backend validation pass.

## Reviewer Reject Signals

- A patch maps aggregate stack sources to scalar loads based only on size,
  fixture names, value ids, stack slot ids, or offsets.
- A patch moves target instruction spelling or concrete register names into
  shared prepared records.
- A patch weakens idea 42 fail-closed coverage without replacing it with
  shared aggregate authority.
- A patch broadens unrelated dynamic, typed-scalar, or pointer-base behavior.
