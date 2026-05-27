Status: Active
Source Idea Path: ideas/open/43_shared_prepared_typed_stack_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Shared Prepared Typed Stack-Source Facts

# Current Packet

## Just Finished

Completed Step 2 by adding `PreparedTypedStackSourcePublication` and
`prepare_same_width_i32_stack_source_publication()` in the shared prepared
lookup layer.

Changed facts:

- The shared query exposes the selected same-width `I32` concrete-offset
  `StackSlot(size_bytes=4) -> Register` authority from `PreparedEdgePublication`
  plus its matched `PreparedMoveResolution`.
- Available records carry source/destination value ids and names, BIR source
  and destination types, `SameWidthNoExtension`, source slot id/offset/size/alignment,
  destination register bank, and full destination register placement.
- Missing or unsupported states are distinct: unavailable publication,
  unsupported source home, missing concrete stack facts, missing same-width
  `I32` types, unsupported destination storage, missing destination register
  placement, unsupported move authority, missing destination `Gpr` bank, and
  missing destination register view.
- Focused prepared lookup-helper coverage now proves the available carrier and
  fail-closed states for missing type, missing concrete stack facts, missing
  placement, unsupported move authority, wrong bank, and incomplete register
  view.

## Suggested Next

Implement Step 3 by consuming
`prepare_same_width_i32_stack_source_publication()` in the RISC-V edge
publication path for the selected same-width `I32` stack-source form.

## Watchouts

RISC-V still has not been changed in this packet. Step 3 should copy only the
new shared typed facts into the target-local intent; do not re-infer type,
extension policy, or destination bank from byte size, ids, offsets, fixture
names, raw register spelling, or local helper shape. The new query currently
supports only same-width `I32`; existing `I64`, subword, widening, F32, dynamic,
and aggregate routes must stay fail-closed unless a matching shared authority
record is added for them.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'`

Result: passed. The accepted proof log was rolled forward to `test_before.log`.

Additional local sanity: `ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'` passed.
