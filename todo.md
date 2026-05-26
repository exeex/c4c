Status: Active
Source Idea Path: ideas/open/12_dispatch_value_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Establish the Named Materialization Owner

# Current Packet

## Just Finished

Step 2 established `prepared_value_home_materialization` as the named owner for
prepared value-home reads and prepared home publication-to-register planning.
Moved `emit_prepared_value_home_to_register` into the new owner header/source,
kept `emit_prepared_scalar_publication_plan_to_register` and
`emit_prepared_home_publication_plan_to_register` private to that translation
unit, and added a narrow owner wrapper used by
`emit_value_publication_to_register`. Direct clients now include
`prepared_value_home_materialization.hpp`, while
`emit_value_publication_to_register` remains declared by
`dispatch_value_materialization.hpp`.

## Suggested Next

Next coherent packet: extract another leaf helper family that still hangs off
`dispatch_value_materialization.cpp`, preferably the prepared FP value
materialization cluster (`emit_fp_immediate_to_register` and
`emit_fp_value_to_register`) while keeping current store, edge-copy, and generic
publication callers stable behind a narrow owner header.

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

## Proof

Proof passed and is recorded in `test_after.log`.

Command run exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_branch_control_lowering|backend_aarch64_return_lowering|backend_aarch64_machine_printer)$' >> test_after.log 2>&1`

Result: build completed and 4/4 focused tests passed. AST-backed checks after
the move confirmed `emit_prepared_value_home_to_register` in
`prepared_value_home_materialization.cpp`, the two plan helpers in that
translation unit's anonymous namespace, and `emit_value_publication_to_register`
still declared by `dispatch_value_materialization.hpp`.
