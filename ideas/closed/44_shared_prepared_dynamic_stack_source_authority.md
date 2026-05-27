# Shared Prepared Dynamic Stack Source Authority

## Goal

Define shared prepared authority for dynamic-address stack-source
edge-publications, including base anchor, address expression, load width, and
scratch requirements, before any target lowers them as `StackSlot -> Register`
loads.

## Why This Exists

`41_riscv_prepared_edge_publication_dynamic_stack_source_policy.md` closed as a
validated fail-closed blocker. RISC-V currently supports concrete stack
offsets, including large-offset materialization, but it has no shared prepared
authority for stack sources whose effective address is dynamic or otherwise
not represented by a concrete `offset_bytes`.

Lowering those cases by assuming an `sp` offset, scanning predecessor or
successor blocks, or reclassifying pointer-base facts as stack loads would
recreate target-local semantic authority.

## In Scope

- Inventory dynamic stack-source shapes that reach prepared edge-publication
  planning.
- Decide the target-neutral record shape for base anchor, offset expression,
  source memory access, load width, and validity status.
- Add shared prepared facts or fail-closed diagnostics for one selected dynamic
  stack-source family.
- Update RISC-V only when enough shared authority exists for safe target
  lowering.
- Add focused tests for the selected supported or explicitly fail-closed
  dynamic form.

## Out Of Scope

- Typed scalar extension/register-bank policy for concrete offsets.
- Aggregate stack-source copy policy.
- Pointer-base publication broadening unrelated to dynamic stack-source
  authority.
- Broad frame-layout or dynamic-stack rewrites.
- Target-local predecessor/successor scans.

## Acceptance Criteria

- Dynamic stack-source prepared facts are either available for one narrow form
  or documented as blocked by a precise upstream producer gap.
- RISC-V does not infer dynamic stack loads from missing concrete offsets,
  pointer-base decorations, fixture shape, value ids, or block scans.
- Existing concrete stack-source behavior remains supported.
- Unsupported dynamic forms remain explicit and fail closed.
- Focused proof and backend validation pass.

## Closure Note

Closed after validating one narrow supported family: same-width i32 dynamic
`StackSlot -> Register` loads whose source is a `LoadLocal`-produced dynamic
stack slot and whose memory authority is carried by shared prepared
source-memory facts. RISC-V consumes only that shared authority, while missing,
incomplete, unavailable, non-i32, aggregate, pointer-base-only, and otherwise
unsupported neighboring forms remain fail-closed. Existing concrete-offset and
large-offset stack-source behavior remains supported.

Validation evidence at close:

- Backend CTest passed 163/163 after the shared-authority and RISC-V consumer
  slices.
- Full CTest passed 3411/3411 for the final validation packet.
- Close-time regression guard compared the accepted full-suite baseline against
  `test_after.log` with no new failures and no lost passes.

## Reviewer Reject Signals

- A patch assumes `sp` plus an offset when shared prepared facts do not provide
  a concrete base/address contract.
- A patch treats `PointerBasePlusOffset` as a stack-source load without a
  source idea explicitly changing that authority.
- A patch hides dynamic support behind target-local source rediscovery.
- A patch weakens the fail-closed cases from idea 41 without replacing them
  with shared prepared authority.
