Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Direct Method And Member Probes Through Helpers

# Current Packet

## Just Finished

Completed the accumulated `plan.md` Step 3 helper-routing packets for direct
method and member probes in `range_for.cpp`, `operator.cpp`, `object.cpp`, and
the `NK_DEREF` branch in `hir_types.cpp`.

The broader HIR checkpoint also passed after those slices:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir > test_after.log`
reported 73/73 passing.

## Suggested Next

Supervisor should choose either Step 4 owner-aware record lookup work or plan
review. Do not send another blind helper-routing packet for the remaining direct
method-map hits without first narrowing an exact safe target.

## Watchouts

- Remaining `struct_methods_` and `struct_method_ret_types_` hits are not safe
  Step 3 conversion targets by default.
- The remaining hits cover helper internals, method registration,
  ref-overload registration, and exact out-of-class method attach/skip checks in
  `hir_build.cpp`.
- Those registration and attach/skip paths require exact rendered keys, while
  `find_struct_method_mangled` can fall back between const/non-const variants
  and inherited bases.

## Proof

Passed:
`git diff --check`

Previously recorded broader HIR checkpoint:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir > test_after.log`
passed 73/73.

Proof log: metadata-only packet, no new `test_after.log` generated; broader
checkpoint log path remains `test_after.log`.
