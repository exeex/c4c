Status: Active
Source Idea Path: ideas/open/28_riscv_prepared_edge_publication_stack_source_policy_broadening.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Stack Source Broadening Options

# Current Packet

## Just Finished

Completed Step 1 inventory for RISC-V stack-source broadening.

Current behavior:

- `src/backend/mir/riscv/codegen/emit.cpp` consumes shared prepared
  `edge_publications` through `consume_edge_publication_move_intent()`.
- `render_edge_publication_source_operand()` currently accepts
  `StackSlot -> Register` sources only when `offset_bytes` is present and
  `size_bytes == 4`.
- The accepted 4-byte stack source records slot id, offset, and size in
  `EdgePublicationMoveIntent`, then renders `lw <dst>, <offset>(sp)`.
- Clearing `publications_by_edge_destination` still returns
  `MissingPublication`, so the path is shared-lookup driven rather than local
  predecessor/successor rediscovery.

Available stack-source facts:

- `PreparedValueHome::slot_id`
- `PreparedValueHome::offset_bytes`
- `PreparedValueHome::size_bytes`
- No scalar type, signedness, floating-point class, register-class contract, or
  dynamic address expression is available in the stack-source home consumed by
  the RISC-V helper.

Candidate forms:

- 8-byte concrete-offset stack source: supportable as a direct
  `ld <dst>, <offset>(sp)` when `offset_bytes` is present and the offset fits
  the RISC-V signed 12-bit load immediate range.
- 1-byte and 2-byte concrete-offset stack sources: blocked because choosing
  `lb`/`lbu` or `lh`/`lhu` requires signedness/type facts that are not present.
- 4-byte unsigned or floating stack sources: blocked because choosing `lw` vs
  `lwu` or GP vs FP load policy requires type/register-class facts that are not
  present. Preserve the existing focused I32 `lw` behavior only.
- Stack sources without concrete offsets or with dynamic addressing: blocked
  because the helper has no address expression or scratch-register policy.
- Large concrete offsets outside signed 12-bit load range: blocked for new
  forms until address materialization and scratch-register policy are explicit.
- Aggregate or 16-byte stack sources: blocked because multi-load, lane order,
  and destination-register-pair policy are not present.

Selected first broadened form:

- Implement 8-byte concrete-offset `StackSlot -> Register` through shared
  `edge_publications`, rendering `ld <dst>, <offset>(sp)` only for a concrete
  offset in the signed 12-bit RISC-V load range.

Implementation targets:

- `src/backend/mir/riscv/codegen/emit.cpp`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

## Suggested Next

Execute Step 2 by adding the 8-byte concrete-offset stack-source policy,
including positive `ld` coverage and negative fail-closed coverage for missing
offset, unsupported widths, and signed-12-bit offset overflow for the new 8-byte
form.

## Watchouts

Keep shared `edge_publications` as the only semantic authority. Do not broaden
sub-word, unsigned 32-bit, floating-point, dynamic-address, aggregate, or large
offset stack-source forms until the missing type/register-class/address/scratch
policy exists. Preserve existing `Register`, `RematerializableImmediate`,
`PointerBasePlusOffset`, 4-byte stack-source, and stack-destination behavior.

## Proof

Proof mode: `docs/inventory-only`.

No build or tests were required for Step 1, and no `test_after.log` was created
per the delegated packet.

Focused proof command for Step 2:

`cmake --build --preset default --target backend_riscv_prepared_edge_publication_test > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' >> test_after.log 2>&1`
