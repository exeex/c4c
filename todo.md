Status: Active
Source Idea Path: ideas/open/202_route3_memory_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Reader And Baseline Proof

# Current Packet

## Just Finished

Step 1 selected `make_unpublished_load_local_source_operand(...)` in
`src/backend/mir/aarch64/codegen/alu.cpp` as the first Route 3 adapter reader.
It is a bounded direct memory/source identity reader because it currently asks
`find_prepared_load_local_source_producer(...)` for same-block `load_local`
source identity before forming a scalar ALU/control memory operand. The Route 3
identity source should be `mir::find_bir_same_block_load_local_source_identity`
over `BirSameBlockLoadLocalSourceRequest`, backed by
`route3_build_memory_access_index(...)` /
`route3_find_same_block_load_local_source(...)`.

The selected reader must treat Route 3 as semantic identity only:
`load_local` producer instruction, memory access id, result value identity,
local-slot source identity, block/instruction compatibility, and fail-closed
same-block ordering. It must not take frame offsets, stack layout, address
formation, base-plus-offset legality, materialization, final operands, or
target-register/home policy from BIR.

Retained prepared fallback: keep
`prepare::find_prepared_same_block_load_local_source_producer(...)` plus the
prepared `PreparedMemoryAccess` consumed by `make_prepared_scalar_load_source`.
That prepared path remains authoritative for stack layout, frame-slot id to
target address, byte offset, size/align, `can_use_base_plus_offset`,
`byte_offset_is_prepared_snapshot`, value homes, and the final
`MemoryOperandSupportKind::Prepared` operand.

## Suggested Next

Implement a small Route 3 identity adapter for
`make_unpublished_load_local_source_operand(...)`: query Route 3 first for the
same-block load-local identity, require agreement with the prepared fallback
before using prepared target-addressing facts, and fall back/return nullopt for
absent Route 3 facts, non-memory/non-load-local roots, type/name mismatch,
instruction mismatch, same-slot ambiguity, or any case where prepared remains
the only target-addressing authority.

## Watchouts

Do not move address formation, frame/global/TLS relocation, stack/frame
offsets, concrete layout, addressing-mode legality, materialization, final
operands, or target-addressing fallback into BIR schema.

Do not replace all `memory_accesses`, `PreparedMemoryAccessLookups`, or
memory/frame/stack helper groups.

Existing coverage already exercises Route 3/prepared load-local source
agreement and fail-closed behavior in `backend_store_source_publication_plan`
and `backend_prepared_lookup_helper`, and the current AArch64 reader success
path in `backend_aarch64_prepared_scalar_alu_records`. Missing before code
changes: focused reader-boundary coverage that proves Route 3/prepared
agreement is required at `make_unpublished_load_local_source_operand(...)`, with
fallback/nullopt on missing Route 3 identity, mismatched route/prepared
instruction or root type/name, non-`load_local` input, same-slot ambiguity or
intervening-store ambiguity, and unchanged prepared target-addressing facts.

## Proof

Read-only Step 1 inspection only; no build/tests run and no canonical logs
touched.

Search/tooling proof run:

- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/memory.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json`
- `c4c-clang-tool function-signatures src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/alu.cpp build/compile_commands.json | rg -n "find_prepared_load_local_source_producer|make_unpublished_load_local_source_operand|load_local_home_needs_consumer_publication|find_unique_indexed_prepared_memory_access|MemoryOperand|prepared"`
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/alu.cpp make_unpublished_load_local_source_operand build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/alu.cpp make_unpublished_load_local_source_operand build/compile_commands.json`
- `rg -n "route3_find|Route3|find_prepared_memory_access|find_unique_indexed_prepared_memory_access|same_block.*global|load_local|stored_value_source|prepared_store_local_access|prepared_memory_access\(" src/backend/mir/aarch64/codegen src/backend/bir tests docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
- `rg -n "backend_prepared_lookup_helper_test|verify_route3_load_local_stored_value_source_matches_prepared_or_falls_back|BIR same-block load-local source query|same-block load-local" tests/backend/bir/CMakeLists.txt tests/backend/bir/backend_prepared_lookup_helper_test.cpp tests -g '*.cpp' -g 'CMakeLists.txt'`
- `ctest --test-dir build -N | rg -n "prepared_lookup_helper|aarch64.*dispatch|backend_prepared|handoff.*multi|x86_handoff"`
- `ctest --test-dir build -N | rg -n "scalar_alu|store_source|publication_plan|aarch64_prepared"`

Proposed implementation proof subset:

`cmake --build build --target backend_aarch64_prepared_scalar_alu_records backend_store_source_publication_plan backend_prepared_lookup_helper && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_scalar_alu_records|backend_store_source_publication_plan|backend_prepared_lookup_helper)$'`

`test_after.log` was not written because this packet explicitly required
read-only inspection and forbade touching canonical logs.
