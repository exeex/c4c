Status: Active
Source Idea Path: ideas/open/12_dispatch_value_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Thin `dispatch.cpp` Materialization Touchpoints

# Current Packet

## Just Finished

Step 4 packet complete. The scalar multiply helper
`lower_scalar_mul_with_distinct_rhs_scratch` now lives in `alu.cpp` and is
declared by `alu.hpp`; it is no longer declared or defined by
`dispatch_value_materialization.*`. `dispatch.cpp` still calls the helper at
the same dispatch ordering point through the scalar ALU owner header.

## Suggested Next

Next coherent packet: continue Step 4 by thinning the remaining narrow
`dispatch.cpp` materialization touchpoint `lower_local_slot_address_publication`
without moving the central `emit_value_publication_to_register` bridge.

## Watchouts

- Keep this route focused on value materialization authority.
- Do not fold in edge-copy cleanup, publication cleanup, calls cleanup, or
  broad AArch64 pipeline redesign.
- Treat expectation weakening, unsupported downgrades, text-emission contracts,
  and named-test shortcuts as route failures.
- Leave unrelated transient `review/` artifacts untouched.
- `emit_value_publication_to_register` is the fan-out point and should not be
  the first move; it recursively consumes most leaf materializers and is used by
  calls, stores, edge copies, publication, and branch fusion hooks.
- The generic dispatcher now calls
  `emit_prepared_value_home_publication_to_register` as the narrow bridge into
  the new owner. Keep the two plan helpers private unless a later packet has a
  concrete direct client.
- The branch-fusion hook struct still carries a function pointer named
  `emit_prepared_value_home_to_register`; the function declaration itself now
  comes from `prepared_value_home_materialization.hpp`.
- `fp_value_materialization.cpp` still depends on generic
  `emit_value_publication_to_register` for integer compare/select and cast
  materialization subpaths; keep that bridge intentional unless a later packet
  splits those dependencies semantically.
- `globals.cpp` now has a private prepared-memory-access lookup equivalent to
  the existing edge-copy helper so the moved global-symbol load leaf does not
  depend on edge-copy ownership.
- `emit_prepared_va_list_field_load_to_register` depends on the existing
  publication helpers for prepared `va_list` field addresses and scalar load
  register spelling; keep those dependencies as imported helper calls unless a
  later packet moves that shared va-list address surface too.
- `globals.cpp` now includes the narrow helper owners needed by the moved GOT
  materialization route; its private memory-access helper was renamed to
  `prepared_global_memory_access` to avoid colliding with the existing
  edge-copy helper declaration.
- `emit_prepared_pointer_value_load_to_register` now imports the same
  dispatch lookup/publication helpers from the memory/store-source owner; keep
  it there unless a later route creates an even narrower prepared pointer-load
  owner.
- `fp_value_materialization.cpp` now owns the prepared scalar FP binary
  publication route helper and imports `dispatch_edge_copies.hpp` only for the
  existing select-chain materialization instruction wrapper.
- `cast_ops.cpp` now owns the two prepared scalar cast publication helpers and
  imports the generic publication/common lookup helpers needed by their existing
  implementation. Keep the central `emit_value_publication_to_register` bridge
  in generic dispatch value materialization unless a later packet explicitly
  owns that fan-out move.
- `memory_store_sources.cpp` now owns the stack-homed pointer-value load
  publication route helper because that route depends on the prepared
  pointer-value load/store-source machinery. Keep dispatch ordering unchanged
  unless a later packet explicitly owns order changes.
- `alu.cpp` now owns `lower_scalar_mul_with_distinct_rhs_scratch` but still
  imports the generic publication bridge for existing operand materialization;
  keep that bridge dependency intentional unless a later packet owns the
  fan-out split.

## Proof

Proof passed and is recorded in `test_after.log`.

Command run exactly:
`cmake --build --preset default > test_after.log 2>&1` followed by
`ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value)$' >> test_after.log 2>&1`

Result: build completed and 5/5 focused tests passed. AST-backed checks before
the move confirmed `lower_scalar_mul_with_distinct_rhs_scratch` was defined in
`dispatch_value_materialization.cpp`, declared from
`dispatch_value_materialization.hpp`, and directly called by
`dispatch_prepared_block` in `dispatch.cpp`. AST-backed checks after the move
confirmed the definition is in `alu.cpp`, the declaration resolves from
`alu.hpp`, and the helper is still directly called by `dispatch_prepared_block`
in `dispatch.cpp`; `rg` confirmed no remaining declaration or definition in
`dispatch_value_materialization.*`.
