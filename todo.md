# Current Packet

Status: Active
Source Idea Path: ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Pointer-Base Path

## Just Finished

Completed Step 1 inventory for the RISC-V pointer-base prepared edge-publication
route. The current consumer is `consume_edge_publication_move_intent` in
`src/backend/mir/riscv/codegen/emit.cpp`: it asks the shared
`find_unique_indexed_prepared_edge_publication` lookup for authority, renders a
source operand locally, requires a register destination home, and emits target
syntax through `append_edge_publication_move_instruction` / `emit_prepared_module`.

`PointerBasePlusOffset` homes expose `pointer_base_value_name` and
`pointer_byte_delta` on `PreparedValueHome`; the prepared lookup state also has
`PreparedValueHomeLookups::value_ids` plus `homes_by_id`, so RISC-V can resolve
the base value name back to a base home without rediscovering edge facts. The
safe first policy boundary is: only accept pointer-base homes whose base value
resolves to a register home with `register_name`, whose delta is present and fits
RISC-V's signed 12-bit `addi` immediate range. That can materialize the address
as `addi <dst>, <base>, <delta>` for register destinations, with a `delta == 0`
form eligible to render as `mv <dst>, <base>`.

Target files for the next code packet are
`src/backend/mir/riscv/codegen/emit.hpp`,
`src/backend/mir/riscv/codegen/emit.cpp`, and
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`.

## Suggested Next

Execute Step 2/3 by recording the narrow pointer-base materialization policy in
the helper intent and implementing `PointerBasePlusOffset -> Register` for
register-base, signed-12-bit-delta homes through shared `edge_publications`
authority.

## Watchouts

Do not implement source-to-`StackSlot` destinations or stack-source policy
broadening in this route. Do not rediscover edge facts locally; the shared
`edge_publications` lookup remains the semantic authority.

Keep these forms explicit and fail closed for this route unless the supervisor
delegates a separate policy packet: missing `pointer_base_value_name`, missing
`pointer_byte_delta`, base homes that are not registers, base names that cannot
be resolved through prepared value-home lookups, deltas outside signed 12-bit
`addi` range, source-to-`StackSlot` destinations, and stack-source policy
broadening.

## Proof

docs/inventory-only; no build or tests run, and no `test_after.log` created.
Focused proof command for the implementation packet:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`.
