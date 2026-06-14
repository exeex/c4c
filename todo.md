Status: Active
Source Idea Path: ideas/open/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the riscv Route 5/Route 3 baseline

# Current Packet

## Just Finished

Completed Step 1: established the riscv Route 5/Route 3 edge-publication
adapter baseline.

Inspected:

- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

AST-backed queries used:

- `c4c-clang-tool-ccdb function-signatures .../src/backend/mir/riscv/codegen/emit.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees .../src/backend/mir/riscv/codegen/emit.cpp consume_edge_publication_move_intent build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees .../src/backend/mir/riscv/codegen/emit.cpp append_edge_publication_move_instruction build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures .../src/backend/mir/query.cpp build/compile_commands.json`

Baseline findings:

- Adapter seam: wrap the current prepared publication lookup and source-home
  interpretation in `riscv::codegen::consume_edge_publication_move_intent()`;
  `append_edge_publication_move_instruction()` should stay a thin output
  appender over an `Available` intent. The route-aware boundary should sit
  between `prepare::find_unique_indexed_prepared_edge_publication()` and the
  existing target-local source/destination rendering branches.
- Current riscv authority is prepared-only. The riscv consumer calls
  `prepare::find_unique_indexed_prepared_edge_publication()`,
  `render_edge_publication_source_operand()`, and target-local policy helpers;
  it does not yet call Route 5 or Route 3 helper APIs.
- Route 5 helper surfaces available for agreement-gating:
  `mir::find_bir_cfg_edge_publication_source_identity()`,
  `mir::find_bir_current_block_join_source_identity()`,
  `bir::route5_build_edge_join_source_index()`,
  `bir::route5_find_cfg_edge_publication()`,
  `bir::route5_find_current_block_join_source()`,
  `bir::route5_cfg_edge_publication_record()`, and
  `bir::route5_current_block_join_source_records()`. The relevant MIR status
  contracts are `BirCfgEdgePublicationSourceStatus` and
  `BirCurrentBlockJoinSourceStatus`; the BIR route status contract is
  `Route5PublicationStatus`.
- Route 3 helper surfaces available for memory-source agreement:
  `mir::find_bir_same_block_load_local_source_identity()`,
  `mir::find_bir_same_block_global_load_access_identity()`,
  `mir::find_bir_same_block_load_local_stored_value_source_identity()`,
  plus `bir::route3_build_memory_access_index()`,
  `bir::route3_find_memory_access_record()`,
  `bir::route3_find_same_block_load_local_source()`,
  `bir::route3_find_same_block_global_load_access()`, and
  `bir::route3_find_same_block_load_local_stored_value_source()`.
- Preserved riscv status/output contracts:
  `EdgePublicationMoveIntentStatus::{Available, MissingSharedLookups,
  MissingPublication, UnsupportedPublication, UnsupportedSourceHome,
  UnsupportedDestinationHome}`; exact instruction text for `mv`, `li`, `lw`,
  `ld`, `sw`, `addi`, large-offset `li/add/lw|ld`, pointer-base materialization,
  and dynamic load-local source forms.
- Positive riscv cases currently covered:
  register-to-register `mv a1, a0`, immediate-to-register `li a1, 42`,
  stack-to-register `lw a1, 16(sp)`, i64 stack-to-register `ld a1, 32(sp)`,
  large stack offsets via `t6`, pointer-base register destinations via
  `addi`/`mv`/`li+add`, register/immediate/stack sources to stack destinations
  via `sw` and `t0`, and dynamic LoadLocal stack source memory via
  `lw a1, 12(s2)`.
- Fail-closed cases currently covered:
  null shared lookups, missing publication lookup, unsupported/non-move
  publication, missing source home, malformed stack source, unsupported
  source widths/subword/unsigned/F32, aggregate-width sources, dynamic stack
  without usable prepared memory access, incomplete source memory access,
  source memory requiring address materialization, missing or non-register
  pointer base, stack destination malformed/large/aliasing/unsupported forms,
  pointer-base stack destinations, pointer-base large-delta aliasing, and
  large-offset policy cases that intentionally stay target-owned.
  `backend_prepared_lookup_helper_test.cpp` also covers Route 5 no-source,
  missing source producer, destination mismatch, duplicate, wrong predecessor,
  wrong successor, and memory-source evidence, plus Route 3 prepared match and
  fallback/rejection behavior for load-local stored-value source identity.

## Suggested Next

Execute Step 2 from `plan.md`: add route diagnostic/oracle coverage before
authority changes. Focus on route-visible rows proving Route 5 edge/source
identity and Route 3 memory-source identity agreement, absence, mismatch,
duplicate/conflict, incomplete memory-source, and fallback while preserving
current riscv wrapper output/status expectations.

## Watchouts

- Preserve exact riscv wrapper instruction text and
  `EdgePublicationMoveIntentStatus` names.
- Treat Route 5 and Route 3 facts as agreement evidence only; riscv keeps
  target-local policy.
- Do not claim prepared lookup, aggregate, or draft 155 demotion readiness.
- The first code seam should be an adapter around
  `consume_edge_publication_move_intent()`, not a replacement of
  `PreparedFunctionLookups` or `PreparedBirModule` publication fields.
- Route 5 can identify edge/current-block publication source facts, including
  memory-source records, but it must not select riscv registers, stack offsets,
  scratch registers, or instruction spelling.
- Route 3 can compare memory-source identity; it must not authorize dynamic
  stack, pointer-base, aggregate-width, subword, unsigned, F32, large-offset,
  or address-materialization policy on its own.

## Proof

Analysis-only packet; no build or CTest required and no `test_after.log`
generated.

Likely first narrow proof command for the next code/test packet:

```bash
cmake --build build --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log
```
