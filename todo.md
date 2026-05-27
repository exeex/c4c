# Current Packet

Status: Active
Source Idea Path: ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair prepared source preservation or placement

## Just Finished

Step 2 stopped before code changes. The scalar cast path can detect that `x13`
is stale for `%t35` after `vaarg.join.39`, but it has no safe replacement
source exposed to `cast_ops.cpp`.

Key facts preserved from the blocker:

- `lower_scalar_cast_publication_to_prepared_stack` publishes
  `%t45.byte_offset.i64` to its prepared stack home.
- It delegates to `emit_value_publication_to_register` for the recursive
  `CastOpcode::SExt` publication of `%t35`.
- Prepared value homes say both `%t35` and `%t49` use `x13`, but block-entry /
  edge publication makes `x13` authoritative for `%t49` at `vaarg.join.39`.
- `%t35` was loaded from `%lv.ap.24` before the va_list gp-offset mutation;
  reloading `%lv.ap.24` at the join would read updated state and would not
  recover the original `%t35`.
- No before-instruction consumer move or exposed prepared lookup currently
  gives scalar cast publication a valid current `%t35` source.

## Suggested Next

Execute Step 3 from `plan.md`: repair prepared source preservation or placement
for the `%t35` source before scalar cast publication consumes it.

Owned files for the next packet:

- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp` only as a consumer if a new or
  newly exposed prepared source query is needed

The packet should choose one semantic route:

- preserve `%t35` before `%t49` overwrites `x13`,
- materialize `%t45.byte_offset.i64` / `%t45` while `%t35` is still live,
- or expose an already-prepared preserved `%t35` source through a shared lookup.

## Watchouts

Do not reload `%lv.ap.24` at the join. The va_list field has already been
updated, so that would be a stale-source fix in the opposite direction.

Do not special-case `%t35`, `%t45`, `%t49`, `vaarg.join.39`, `myprintf`, or
`00204`. The general rule is that scalar cast/ALU publication must not trust an
emitted register mapping after prepared block-entry or edge publication has
made the same physical register authoritative for a different value.

Do not accept or commit the dirty `memory.cpp` and `dispatch_edge_copies.cpp`
changes as part of this lifecycle reset. They remain incomplete context that
removed the original `00204` segfault and bad cursor reload, but focused proof
still fails `00204`.

If the prepared preservation/placement owner is outside idea 55's bounded
surface, stop for a lifecycle split instead of broadening the implementation
packet.

## Proof

Last proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: `7/8` passed. The only failing test is
`c_testsuite_aarch64_backend_src_00204_c`, still failing as
`[RUNTIME_MISMATCH]`.

Proof log path: `test_after.log`.
