Status: Active
Source Idea Path: ideas/open/61_aarch64_shared_same_block_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Migrate Recursive Value Materialization

# Current Packet

## Just Finished

Step 4 - Migrate Recursive Value Materialization completed the recursive
same-block materialization migration onto prepared/shared facts and preserved
the stack-sourced current-instruction cast publication route.

Completed:

- Removed the remaining raw
  `mir::evaluate_same_block_integer_constant(...)` calls from recursive value
  materialization and routed same-block integer folding through
  `prepare::evaluate_prepared_same_block_integer_constant(...)`.
- Made same-block scalar producer lookup in this file require
  `context.function.prepared_lookups` instead of constructing source-producer
  lookups locally.
- Bounded recursive binary operand materialization and scratch hazard checks to
  the prepared producer instruction index, keeping AArch64-owned register
  spelling, scratch selection, sequencing, and instruction emission local.
- Updated the materialized-compare branch fixture to attach prepared function
  lookups now that the recursive path fails closed without them.
- Attached prepared lookup authority to instruction-dispatch fixtures that
  directly exercise recursive selected/global/call-argument publication routes,
  including the direct stack-sourced cast publication case.
- Tightened `lower_scalar_cast_instruction(...)` so simple integer casts with
  unstructured sources do not publish selected scalar nodes that the printer
  cannot spell; those cases continue to route through semantic materialization.

## Suggested Next

Supervisor review and commit of the completed Step 4 slice.

## Watchouts

The recursive same-block migration now fails closed when prepared source
producer lookups are absent. Direct unit-test block contexts that exercise
recursive materialization must attach `PreparedFunctionLookups`; production
traversal already carries prepared lookup authority.

## Proof

`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Passed: 169/169 backend tests. Proof log: `test_after.log`.
