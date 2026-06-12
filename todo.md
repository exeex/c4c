Status: Active
Source Idea Path: ideas/open/207_route3_memory_source_semantic_reader.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Select The Reader And Baseline Surface

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting exactly one AArch64 Route 3 memory/source semantic reader for later implementation: `prepared_current_global_load_access(...)` in `src/backend/mir/aarch64/codegen/globals.cpp`.

Selection evidence:
- Selected reader: `prepared_current_global_load_access(context, instruction_index, load_global)`, the globals.cpp helper that currently calls `prepare::find_prepared_global_load_access(...)` and returns `PreparedSameBlockGlobalLoadAccess` for the current BIR `LoadGlobalInst`.
- Why this is semantic source identity: the reader answers whether the current same-block global load has a matching memory/source identity: load instruction, result value, global symbol identity, block, and instruction index. Its only current caller, `make_load_global_got_materialization_instruction(...)`, performs address-policy selection separately through `prepared_load_global_address_policy(...)`, so Route 3 can be limited to identity agreement and must not choose GOT/direct relocation, offsets, scratch registers, final operands, or emitted lines.
- Prepared fallback surface: keep the existing prepared answer from `prepare::find_prepared_global_load_access(context.function.prepared->names, addressing, context.control_flow_block->block_label, instruction_index, load_global)`. If Route 3 is absent, invalid, mismatched, non-global, wrong-type, or disagrees with prepared `PreparedMemoryAccess`, return the same prepared result or `std::nullopt` that the current helper would return.
- Route 3 evidence source: `mir::find_bir_same_block_global_load_access_identity(...)` over `BirSameBlockGlobalLoadAccessRequest` for `context.bir_block`, current block label, `load_global.result`, and the current instruction index; underneath it uses `bir::route3_build_memory_access_index(...)` and `bir::route3_find_same_block_global_load_access(...)`. Agreement should compare at least load instruction index, result value name/type, block label, node kind `LoadGlobal`, base kind `GlobalSymbol`, global name/id, address space, and volatility against the prepared access.
- Expected no-change surfaces: `prepared_load_global_address_policy(...)`, `emit_prepared_global_load_to_register(...)`, `emit_prepared_global_symbol_load_to_register(...)`, address materialization records, relocation spelling, target register/scratch selection, x86 wrappers, prepared printer/debug rows, helper-oracle strings, expected-output baselines, and all prepared memory/addressing public APIs remain unchanged.

## Suggested Next

Execute `plan.md` Step 2 for only `prepared_current_global_load_access(...)`: add an agreement-gated Route 3 read that can corroborate the prepared same-block global-load identity, while retaining prepared fallback for every absent, invalid, or mismatch path.

## Watchouts

- Keep the slice to one semantic reader; do not merge in Route 3 diagnostic/oracle row work from idea 208.
- Route 3 may supply source identity only under route/prepared agreement.
- Prepared memory/addressing remains authoritative for address formation, frame/global/TLS relocation, offsets, address spaces, materialization, final operands, and target policy.
- Do not refresh baselines, weaken supported-path contracts, rename helpers, rewrite expected strings, migrate wrappers, or remove prepared fallback.
- Do not select `make_memory_record_from_prepared_access(...)` or `make_prepared_scalar_load_source(...)` for this Route 3 packet; those construct final memory operands from prepared address/home policy and are broader than a semantic source-identity reader.
- The selected helper is adjacent to GOT-required global-load materialization, so Step 2 must prove Route 3 never decides GOT-vs-direct policy or emitted relocation strings.

## Proof

Analysis-only Step 1 packet; no build required and no `test_after.log` produced.

Recommended narrow proof command for the later implementation packet:
`cmake --build build --target backend_aarch64_prepared_memory_operand_records backend_aarch64_instruction_dispatch && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log`

The first test covers Route 3 global-load identity agreement, wrong-type/non-global rejection, and prepared oracle parity. The second test should be included because the selected reader feeds global-load materialization paths whose GOT/direct relocation ordering and expected emitted assembly must remain byte-stable.
