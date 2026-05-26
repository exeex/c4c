Status: Active
Source Idea Path: ideas/open/12_dispatch_value_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Misowned Helpers to Existing Narrow Owners

# Current Packet

## Just Finished

Step 3 moved the prepared global-symbol scalar load leaf helper into the
existing global/address materialization owner. `emit_prepared_global_symbol_load_to_register`
now lives in `globals.cpp` and is declared by `globals.hpp`; it is no longer
declared or defined by `dispatch_value_materialization.*`. The generic
`emit_value_publication_to_register` path still calls it through the narrower
`globals.hpp` owner without changing behavior.

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

## Proof

Proof passed and is recorded in `test_after.log`.

Command run exactly:
`cmake --build --preset default > test_after.log 2>&1` followed by
`ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_aarch64_machine_printer|backend_aarch64_return_lowering|backend_aarch64_instruction_dispatch)$' >> test_after.log 2>&1`

Result: build completed and 4/4 focused tests passed. AST-backed checks after
the move confirmed `emit_prepared_global_symbol_load_to_register` is defined in
`globals.cpp`, declared by `globals.hpp`, no longer declared by
`dispatch_value_materialization.hpp`, and still reached by
`emit_value_publication_to_register` through the narrow owner.
