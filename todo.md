# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish pointer-select aggregate-copy baseline and probes

## Just Finished

Step 1 established the dispatch-route baseline for the remaining pointer-select
aggregate-copy failure and registered both backend route probes.

Added two source probes under `tests/backend/case/`:

- `aarch64_pointer_select_aggregate_byte_copy.c` covers a branch-selected local
  aggregate pointer loaded after the join and consumed by byte-copy loads from
  `addr %t33+N`.
- `aarch64_variadic_aggregate_overflow_byte_copy.c` covers repeated 9-byte
  aggregate `va_arg` copies. Prepared BIR converts the va_arg joins into
  pointer-valued selects such as `%t33`, `%t51`, `%t69`, and `%t87`, each
  feeding `addr %tNN+N` byte-copy loads.

Fresh `00204` observations:

- BIR has `%t49 = bir.phi ptr [vaarg.reg.38, %t45] [vaarg.stack.36, %t47]`
  followed by byte-copy loads from `addr %t49` through `addr %t49+8`.
- Prepared BIR rewrites that into `%t49.phi.sel0` and `%t49` pointer-valued
  selects; the byte-copy loads still consume `addr %t49+N`.
- Assembly still materializes the selected pointer through a 32-bit truncating
  path at `vaarg.join.39`: `mov w9, w13; sxtw x9, w9`, then continues with
  byte loads from `x13`.
- The authoritative source should remain the prepared selected pointer value
  itself, backed by prepared value-home/select-chain facts for the join result,
  not a locally rebuilt integer scratch value.

Registered both probes in `tests/backend/CMakeLists.txt` with the existing
`c4c_add_backend_codegen_route_test` helper. `ctest -N -R` now lists all eight
focused tests, including:

- `backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy`
- `backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy`

## Suggested Next

Delegate a code packet owning `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
and any directly required helper headers to make non-edge pointer-valued select
materialization load/publish the prepared pointer home directly for aggregate
byte-copy address consumers, preserving the already-present cursor and edge-copy
context without broadening those implementation edits.

## Watchouts

Do not treat the current `memory.cpp` and `dispatch_edge_copies.cpp` changes as
committable merely because they removed the original cursor crash. Preserve the
fixed overflow-block shape while investigating `%t49`, but keep the slice
classified as incomplete until the pointer materialization mismatch is repaired
and proven.

Keep `ideas/open/52_aarch64_calls_prepared_authority_repair.md` open and parked.
This reset does not close the calls route or the earlier ALU route.

The plain pointer probe initially failed semantic BIR lowering when the selected
pointer targeted global aggregates and when the aggregate was 9 bytes; the
representable same-feature source uses local 4-byte aggregates. The variadic
probe preserves the 9-byte overflow-copy shape and reproduces the pointer-select
byte-copy lowering directly.

The two new probes pass as route-generation probes with `.text` required
snippets. They are semantic source-shape coverage, not final correctness proof
for the still-failing `00204` runtime case.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: 7/8 passed. Passing tests were the two dispatch value-materialization
probes, the two ALU prepared-authority probes, `00164`, `00176`, and `00181`;
failing test was `c_testsuite_aarch64_backend_src_00204_c` with
`[RUNTIME_MISMATCH]`.

Proof log: `test_after.log`.
