Status: Active
Source Idea Path: ideas/open/12_dispatch_value_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Misowned Helpers to Existing Narrow Owners

# Current Packet

## Just Finished

Step 3 moved the LoadGlobal GOT materialization route helper into the existing
globals/address owner. `make_load_global_got_materialization_instruction` now
lives in `globals.cpp` and is declared by `globals.hpp`; it is no longer
declared or defined by `dispatch_value_materialization.*`. `dispatch.cpp`
continues to reach the helper through the globals owner without behavior
changes.

## Suggested Next

Next coherent packet: continue Step 3 by extracting another clearly owned leaf
family still hanging off `dispatch_value_materialization.cpp`, or route to
review if the supervisor wants an ownership check before more moves.

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

## Proof

Proof passed and is recorded in `test_after.log`.

Command run exactly:
`cmake --build --preset default > test_after.log 2>&1` followed by
`ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_aarch64_machine_printer|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch)$' >> test_after.log 2>&1`

Result: build completed and 4/4 focused tests passed. AST-backed checks before
the move confirmed `make_load_global_got_materialization_instruction` was
declared by `dispatch_value_materialization.hpp`, defined in
`dispatch_value_materialization.cpp`, and directly called by
`dispatch_prepared_block`. AST-backed checks after the move confirmed it is
declared by `globals.hpp`, defined in `globals.cpp`, no longer appears in
`dispatch_value_materialization.*`, and is still directly reached by
`dispatch_prepared_block`.
