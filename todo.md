Status: Active
Source Idea Path: ideas/open/prealloc-responsibility-map-and-layout-plan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Prealloc Files And Inputs

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: recursive audit of `src/backend/prealloc`
files, sizes, broad surfaces, suspicious helper names, first observations, and
the recommended Step 2 handoff.

### Recursive `.cpp` / `.hpp` Inventory

Total: 109 files, 31,635 lines: 60 `.cpp`, 49 `.hpp`.

```text
243  src/backend/prealloc/addressing.hpp
179  src/backend/prealloc/atomics.cpp
9    src/backend/prealloc/atomics.hpp
1653 src/backend/prealloc/call_plans.cpp
9    src/backend/prealloc/call_plans.hpp
300  src/backend/prealloc/calls.hpp
3156 src/backend/prealloc/control_flow.hpp
312  src/backend/prealloc/decoded_home_storage.cpp
197  src/backend/prealloc/decoded_home_storage.hpp
131  src/backend/prealloc/dynamic_stack.cpp
13   src/backend/prealloc/dynamic_stack.hpp
1181 src/backend/prealloc/f128_runtime_helpers.cpp
9    src/backend/prealloc/f128_runtime_helpers.hpp
134  src/backend/prealloc/formal_publications.cpp
107  src/backend/prealloc/formal_publications.hpp
354  src/backend/prealloc/frame.hpp
350  src/backend/prealloc/frame_plan.cpp
9    src/backend/prealloc/frame_plan.hpp
1477 src/backend/prealloc/i128_runtime_helpers.cpp
9    src/backend/prealloc/i128_runtime_helpers.hpp
574  src/backend/prealloc/inline_asm.cpp
9    src/backend/prealloc/inline_asm.hpp
567  src/backend/prealloc/intrinsics.cpp
9    src/backend/prealloc/intrinsics.hpp
103  src/backend/prealloc/label_identity.cpp
9    src/backend/prealloc/label_identity.hpp
471  src/backend/prealloc/legalize.cpp
1013 src/backend/prealloc/liveness.cpp
84   src/backend/prealloc/liveness.hpp
458  src/backend/prealloc/module.hpp
154  src/backend/prealloc/names.hpp
1568 src/backend/prealloc/out_of_ssa.cpp
72   src/backend/prealloc/prealloc.cpp
3    src/backend/prealloc/prealloc.hpp
425  src/backend/prealloc/prepared_lookups.cpp
122  src/backend/prealloc/prepared_lookups.hpp
66   src/backend/prealloc/prepared_printer.cpp
13   src/backend/prealloc/prepared_printer.hpp
139  src/backend/prealloc/prepared_printer/addressing.cpp
123  src/backend/prealloc/prepared_printer/atomics.cpp
364  src/backend/prealloc/prepared_printer/calls.cpp
262  src/backend/prealloc/prepared_printer/control_flow.cpp
202  src/backend/prealloc/prepared_printer/frame.cpp
537  src/backend/prealloc/prepared_printer/functions.cpp
104  src/backend/prealloc/prepared_printer/inline_asm.cpp
157  src/backend/prealloc/prepared_printer/intrinsics.cpp
28   src/backend/prealloc/prepared_printer/private.hpp
108  src/backend/prealloc/prepared_printer/regalloc.cpp
734  src/backend/prealloc/prepared_printer/runtime_helpers.cpp
171  src/backend/prealloc/prepared_printer/special_carriers.cpp
129  src/backend/prealloc/prepared_printer/storage.cpp
177  src/backend/prealloc/prepared_printer/value_locations.cpp
302  src/backend/prealloc/prepared_printer/variadic.cpp
212  src/backend/prealloc/publication_plans.cpp
231  src/backend/prealloc/publication_plans.hpp
943  src/backend/prealloc/regalloc.cpp
284  src/backend/prealloc/regalloc.hpp
53   src/backend/prealloc/regalloc/assignment.cpp
67   src/backend/prealloc/regalloc/assignment.hpp
585  src/backend/prealloc/regalloc/call_moves.cpp
26   src/backend/prealloc/regalloc/call_moves.hpp
514  src/backend/prealloc/regalloc/call_return_abi.cpp
60   src/backend/prealloc/regalloc/call_return_abi.hpp
108  src/backend/prealloc/regalloc/classification.cpp
40   src/backend/prealloc/regalloc/classification.hpp
123  src/backend/prealloc/regalloc/consumer_moves.cpp
18   src/backend/prealloc/regalloc/consumer_moves.hpp
82   src/backend/prealloc/regalloc/intervals.cpp
33   src/backend/prealloc/regalloc/intervals.hpp
194  src/backend/prealloc/regalloc/move_records.cpp
63   src/backend/prealloc/regalloc/move_records.hpp
203  src/backend/prealloc/regalloc/phi_moves.cpp
17   src/backend/prealloc/regalloc/phi_moves.hpp
333  src/backend/prealloc/regalloc/pointer_carriers.cpp
29   src/backend/prealloc/regalloc/pointer_carriers.hpp
788  src/backend/prealloc/regalloc/runtime_helpers.cpp
22   src/backend/prealloc/regalloc/runtime_helpers.hpp
89   src/backend/prealloc/regalloc/spill_reload.cpp
19   src/backend/prealloc/regalloc/spill_reload.hpp
60   src/backend/prealloc/regalloc/stack_slots.cpp
28   src/backend/prealloc/regalloc/stack_slots.hpp
58   src/backend/prealloc/regalloc/storage.cpp
23   src/backend/prealloc/regalloc/storage.hpp
358  src/backend/prealloc/regalloc/value_homes.cpp
42   src/backend/prealloc/regalloc/value_homes.hpp
145  src/backend/prealloc/regalloc/values.cpp
25   src/backend/prealloc/regalloc/values.hpp
116  src/backend/prealloc/regalloc_placement_identity.cpp
9    src/backend/prealloc/regalloc_placement_identity.hpp
632  src/backend/prealloc/runtime_helpers.hpp
309  src/backend/prealloc/special_carriers.cpp
343  src/backend/prealloc/special_carriers.hpp
433  src/backend/prealloc/stack_layout/alloca_coalescing.cpp
185  src/backend/prealloc/stack_layout/analysis.cpp
1442 src/backend/prealloc/stack_layout/coordinator.cpp
107  src/backend/prealloc/stack_layout/copy_coalescing.cpp
26   src/backend/prealloc/stack_layout/inline_asm.cpp
54   src/backend/prealloc/stack_layout/regalloc_helpers.cpp
347  src/backend/prealloc/stack_layout/slot_assignment.cpp
62   src/backend/prealloc/stack_layout/stack_layout.hpp
43   src/backend/prealloc/storage.hpp
263  src/backend/prealloc/storage_plans.cpp
9    src/backend/prealloc/storage_plans.hpp
614  src/backend/prealloc/target_register_profile.cpp
54   src/backend/prealloc/target_register_profile.hpp
408  src/backend/prealloc/value_locations.hpp
263  src/backend/prealloc/variadic.hpp
943  src/backend/prealloc/variadic_entry_plans.cpp
9    src/backend/prealloc/variadic_entry_plans.hpp
```

### Largest Files

- Largest implementation files: `call_plans.cpp` 1,653; `out_of_ssa.cpp`
  1,568; `i128_runtime_helpers.cpp` 1,477; `stack_layout/coordinator.cpp`
  1,442; `f128_runtime_helpers.cpp` 1,181; `liveness.cpp` 1,013;
  `regalloc.cpp` and `variadic_entry_plans.cpp` 943 each.
- Largest headers: `control_flow.hpp` 3,156; `runtime_helpers.hpp` 632;
  `module.hpp` 458; `value_locations.hpp` 408; `frame.hpp` 354;
  `special_carriers.hpp` 343; `calls.hpp` 300; `regalloc.hpp` 284;
  `variadic.hpp` 263; `addressing.hpp` 243; `publication_plans.hpp` 231.

### Broad Headers And Include-Heavy Surfaces

- `module.hpp`: 458 lines, 25 includes. Central aggregate that pulls together
  addressing, calls, control flow, decoded storage, publications, frame,
  liveness, regalloc, runtime helpers, storage, values, variadic, BIR, and
  target profile facts.
- `control_flow.hpp`: 3,156 lines, 13 includes. Very large route and
  control-flow contract surface; it also carries move bundle/label identity
  details and several route-local comments.
- `runtime_helpers.hpp`: 632 lines, 12 includes. Broad helper carrier surface
  spanning calls, frame, names, regalloc, special carriers, and value locations.
- `calls.hpp`: 300 lines, 11 includes. Includes frame, names, regalloc, value
  locations, and BIR; preservation route data likely ties call ABI and value
  homes together.
- `regalloc.hpp`: 284 lines, 11 includes. Public regalloc contract includes
  liveness, frame, names, BIR, and target profile facts.
- `publication_plans.hpp`: 231 lines, 10 includes. Publication-facing contract
  combines addressing, calls, frame, names, value locations, and BIR.
- `decoded_home_storage.hpp`: 197 lines, 10 includes. Crosses prepared
  lookups, regalloc, storage, and value locations.
- `prealloc.cpp`: only 72 lines but 17 includes, making it a phase coordinator
  surface rather than a large data contract.
- `regalloc.cpp`: 943 lines and 24 includes. Includes many split
  `regalloc/*` helpers plus dynamic stack, target register profile, and stack
  layout helpers; likely a dispatch/coordinator file for Step 2 classification.
- `prepared_printer/private.hpp`: only 28 lines and 2 includes, but it imports
  `module.hpp`, so each prepared-printer family effectively sees the aggregate
  module contract.

### Suspicious Helper / Route Names

- `fallback_type_size` in `src/backend/prealloc/stack_layout/stack_layout.hpp`
  is used repeatedly by `stack_layout/coordinator.cpp`.
- `fallback_symbol_name`, `fallback_symbol_link_name_id`, and
  `fallback_byte_offset` are local address-planning parameters in
  `stack_layout/coordinator.cpp`.
- `PrepareRoute` and `prepare_route_name` live in `control_flow.hpp`; route
  state is stored in `module.hpp` and printed by `prepared_printer.cpp`.
- `PreparedCallPreservationRoute` and
  `prepared_call_preservation_route_name` live in `calls.hpp`; preservation
  route values are used by call/runtime-helper planning and prepared-printer
  output.
- `PreparedValueKind::Temporary` is a core value-kind spelling in
  `liveness.hpp`, `liveness.cpp`, `regalloc.hpp`, and `regalloc/values.cpp`.
  It is not by itself a workaround, but it is a broad value-classification
  term for Step 2 ownership mapping.
- Route-local bridge/legacy comments are present in `control_flow.hpp`,
  `value_locations.hpp`, and `prepared_printer/calls.cpp`; these are the
  clearest Step 2 watch points for temporary compatibility boundaries.
- `stack_layout/analysis.cpp` mentions a retained Rust route in a comment; this
  is an observation only, not a `.cpp`/`.hpp` helper name.

### First Observations

- `control_flow.hpp` is the clear largest outlier and likely mixes durable
  control-flow contracts with route-local naming, move bundle, and diagnostic
  bridge material.
- `module.hpp` is the highest fan-in aggregate header and should be classified
  separately from narrower family contracts rather than treated as one
  ownership category.
- `runtime_helpers.hpp`, `calls.hpp`, `regalloc.hpp`, and
  `decoded_home_storage.hpp` appear to be the main mixed data-contract
  candidates for Step 2.
- The implementation bulk is concentrated in planner/coordinator files:
  `call_plans.cpp`, `out_of_ssa.cpp`, `i128_runtime_helpers.cpp`,
  `stack_layout/coordinator.cpp`, `f128_runtime_helpers.cpp`, `liveness.cpp`,
  `regalloc.cpp`, and `variadic_entry_plans.cpp`.
- `prepared_printer/` is already family-split and mostly mirrors data families;
  its aggregate dependency enters through `private.hpp` -> `module.hpp`.
- `regalloc/` is already helper-split, but `regalloc.cpp` remains a broad
  coordinator that wires many helper families together.
- The only explicit fallback helpers found are stack-layout size and address
  fallback paths; route/bridge/legacy wording is mostly diagnostic or
  transition commentary and should be evaluated as ownership boundaries before
  any file movement is proposed.

## Suggested Next

Execute Step 2 from `plan.md`: classify every file above into one primary
responsibility category, mark deliberately mixed boundaries, and separate
target-neutral, target-parameterized, and accidentally target-specific
responsibilities. Start with the broad headers and largest planner/coordinator
files listed above, then verify the already split `prepared_printer/`,
`regalloc/`, and `stack_layout/` families against the working model.

## Watchouts

- Do not convert Step 2 into implementation movement; this runbook is still a
  discovery and planning pass.
- Treat `control_flow.hpp`, `module.hpp`, `runtime_helpers.hpp`, `calls.hpp`,
  `regalloc.hpp`, `publication_plans.hpp`, and `decoded_home_storage.hpp` as
  likely mixed surfaces until each contract is classified.
- Treat fallback/route/bridge/legacy names as audit signals, not automatic
  defects. Some are durable preparation concepts; Step 2 should decide which
  ones are temporary compatibility boundaries.
- Preserve target-specific instruction emission and final register spelling
  outside prealloc.

## Proof

Delegated proof: `git diff --check`. This audit-only packet has no build or
test subset. Result: passed.
