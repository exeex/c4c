Status: Active
Source Idea Path: ideas/open/27_riscv_prepared_edge_publication_stack_destination_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Stack Destination Path

# Current Packet

## Just Finished

Completed Step 1 inventory for the RISC-V stack-destination
edge-publication path.

Current consumer path:

- `emit_prepared_module` builds shared `PreparedFunctionLookups`, iterates
  `lookups.edge_publications.publications`, and calls
  `append_edge_publication_move_instruction`.
- `append_edge_publication_move_instruction` delegates to
  `consume_edge_publication_move_intent`, which uses
  `find_unique_indexed_prepared_edge_publication` as the only edge-move
  authority.
- Source rendering currently lives in
  `render_edge_publication_source_operand`; supported register-destination
  source families are `Register`, `RematerializableImmediate`, focused
  4-byte `StackSlot`, and narrow `PointerBasePlusOffset`.
- Destination checking currently rejects anything except
  `PreparedValueHomeKind::Register` with a concrete register name, so the next
  implementation packet should branch there for selected stack destinations
  instead of changing shared prepare.

Selected first source-home family: `Register -> StackSlot`.

Stack destination facts available to the RISC-V backend:

- The shared publication exposes `publication->destination_home`,
  `publication->destination_home_kind`, and
  `publication->destination_storage_kind`.
- A `StackSlot` destination home carries `slot_id`, `offset_bytes`,
  `size_bytes`, and optional `align_bytes` through `PreparedValueHome`.
- The first store policy should require `destination_home->kind ==
  StackSlot`, concrete `offset_bytes`, and `size_bytes == 4`, then emit the
  narrow target-local I32 store `sw <src>, <offset>(sp)`.

Target helper and test files:

- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

## Suggested Next

Execute Step 2 by extending the RISC-V edge-publication consumer to support
`Register -> StackSlot` when shared publication authority is present and the
stack destination has concrete 4-byte store facts.

## Watchouts

Keep stack-destination support driven by shared `edge_publications`; do not scan
predecessor/successor blocks or infer local copies. Preserve the existing
register-destination consumers and keep `RematerializableImmediate ->
StackSlot`, `StackSlot -> StackSlot`, `PointerBasePlusOffset -> StackSlot`,
malformed stack destinations without offset/4-byte size, non-move
publications, and missing shared lookup authority fail-closed until separately
selected.

## Proof

docs/inventory-only; no build or tests run and no `test_after.log` created.

Recommended focused proof command for the Step 2 implementation packet:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1
```
