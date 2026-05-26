Status: Active
Source Idea Path: ideas/open/12_dispatch_value_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Misowned Helpers to Existing Narrow Owners

# Current Packet

## Just Finished

Step 3 moved the prepared FP value materialization leaf cluster into the new
`fp_value_materialization` owner. `emit_fp_immediate_to_register` and
`emit_fp_value_to_register` now live in `fp_value_materialization.cpp` and are
declared by `fp_value_materialization.hpp`; `dispatch_value_materialization.hpp`
no longer declares those FP leaf helpers. The generic
`emit_value_publication_to_register` path still works by including the narrow FP
owner, and direct store/edge-copy clients include the narrower header.

## Suggested Next

Next coherent packet: continue Step 3 by extracting another clearly owned leaf
family still hanging off `dispatch_value_materialization.cpp`, if the
supervisor wants another narrow-owner move before route review.

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

## Proof

Proof passed and is recorded in `test_after.log`.

Command run exactly:
`cmake --build --preset default > test_after.log 2>&1` followed by
`ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result|backend_aarch64_return_lowering|backend_aarch64_instruction_dispatch)$' >> test_after.log 2>&1`

Result: build completed and 4/4 focused tests passed. AST-backed checks after
the move confirmed `emit_fp_immediate_to_register` and
`emit_fp_value_to_register` are defined in `fp_value_materialization.cpp`,
`emit_value_publication_to_register` remains defined in
`dispatch_value_materialization.cpp`, and
`dispatch_value_materialization.hpp` no longer declares the FP leaf helpers.
