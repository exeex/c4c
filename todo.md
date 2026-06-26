Status: Active
Source Idea Path: ideas/open/376_rv64_object_route_prepared_move_bundle_target_shapes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Focused Backend Coverage

# Current Packet

## Just Finished

Completed Steps 2-4: added focused RV64 object-emission coverage for a prepared
scalar integer stack-slot-to-GPR value move bundle, implemented semantic
emission in `fragment_for_prepared_move_bundle()`, and validated the focused
backend subset.

The admitted shape is a one-move prepared bundle with register destination
storage, destination contiguous width `1`, no cycle-temp source, a scalar
integer stack-slot source home with a prepared frame offset, and a scalar GPR
destination home. The emission rejects pointer-typed stack-slot sources for
this packet, then uses the source BIR integer type and prepared stack layout to
select an RV64 `lb`/`lh`/`lw`/`ld` load from `sp + offset`.

Focused tests now prove the positive `i16` stack slot to `s1` move shape and
keep adjacent unsupported shapes fail-closed: stack-slot destinations,
multi-lane destination width, cycle-temp source, missing source stack-slot
metadata, non-integer source type, and pointer-typed stack-slot source.

## Suggested Next

Execute Step 5 from `plan.md`: rerun the single `src/20000217-1.c` RV64
gcc-torture backend representative and record whether it passes or advances to
the next distinct owner.

## Watchouts

- Do not reopen scalar compare/trunc lowering from idea 375.
- Do not infer move semantics from testcase names, hard-coded value ids, source
  syntax, instruction indexes, or the exact representative expression.
- Stack-slot destinations, stack-to-stack moves, multi-lane destinations,
  cycle-temp moves, non-integer sources, FPR destinations, aggregate homes,
  missing homes, and ambiguous destinations are still unsupported unless a
  later packet admits them explicitly.
- The implementation is intentionally phase-agnostic inside the move-bundle
  fragment helper, but object traversal still controls which move-bundle phases
  reach this object-emission path.

## Proof

Command:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: passed. Canonical proof log: `test_after.log`.
