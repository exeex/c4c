Status: Active
Source Idea Path: ideas/open/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Fail-Closed And Compatibility Behavior

# Current Packet

## Just Finished

Step 3 recorded the fail-closed and compatibility proof matrix for the accepted
Step 2 code slice. The matrix is tied to existing assertions in
`tests/backend/bir/backend_x86_route_debug_test.cpp`,
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and the green
`test_after.log` proof.

Proof matrix:

- Positive Route 6 agreement: `backend_x86_route_debug_test.cpp` asserts the
  selected trace contains exactly
  `route6 scalar arg call#0 block=entry inst#2 callee=addip0 arg#0 source=%t1 kind=ArgumentValue`;
  `backend_prepared_lookup_helper_test.cpp` asserts
  `find_consumed_scalar_i32_call_argument_source(...)` returns an
  `ArgumentValue` source, passes
  `route6_call_argument_source_matches_argument_value_record(...)`, exposes
  source name `%x86.scalar`, and keeps prepared `source_value_id` `301`.
- Absent Route 6 source facts: x86 route-debug fixture
  `selected_route6_scalar_arg_without_route6_facts()` clears `arg_sources` and
  asserts the trace header/function row remain while the selected Route 6 row is
  absent; the helper test clears `ConsumedPlans.route6_call_use_sources`,
  asserts `find_consumed_scalar_i32_call_argument_source(...)` fails closed, and
  asserts `find_consumed_call_argument_plan(...)` still finds the prepared
  argument plan.
- Invalid source facts: x86 route-debug fixture
  `selected_route6_scalar_arg_with_invalid_source_id()` clears the Route 6
  `source_value_id` and asserts trace fallback with no selected Route 6 row.
  The public Route 6 helper coverage in `backend_prepared_lookup_helper_test.cpp`
  separately records `MissingSourceRelationship`, `MissingSourceProducer`,
  `WrongCall`, `NoMatch`, `MissingArgument`, and `AbiBoundExcluded` status paths
  as fail-closed.
- Duplicate/conflict source facts: x86 route-debug fixture
  `selected_route6_scalar_arg_with_duplicate_conflict()` adds a conflicting
  duplicate source record and asserts fallback with no selected row. The helper
  test also asserts duplicate call-argument source relationships return
  `DuplicateRelationship` and leave the migrated/public materialization lookup
  unavailable.
- Prepared mismatch: x86 route-debug fixture
  `selected_route6_scalar_arg_with_prepared_mismatch()` changes the prepared
  argument `source_value_id` and asserts fallback with no selected Route 6 row.
  The helper test changes the prepared source id from `301` to `302`, asserts
  `find_consumed_scalar_i32_call_argument_source(...)` fails closed, and asserts
  `find_consumed_call_argument_plan(...)` still preserves prepared selection.
- Same-id source-record mismatch: x86 route-debug fixture
  `selected_route6_scalar_arg_with_source_name_mismatch()` changes the Route 6
  source name to `%t4` while preserving same-id shape and asserts fallback with
  no selected row. The helper test changes `%x86.scalar` to `%other.scalar` and
  asserts the x86 helper rejects same-id local inference without Route 6
  `ArgumentValue` record compatibility.
- Public prepared fallback and `ConsumedPlans` compatibility: helper assertions
  prove the x86 path still goes through `ConsumedPlans`, keeps prepared
  `find_consumed_call_argument_plan(...)` available when Route 6 is absent or
  source ids disagree, and rejects the Route 6-specific source only after
  checking both Route 6 record compatibility and prepared source-id agreement.
- Wrapper output stability: the x86 route-debug test keeps the wrapper trace
  assertions for direct extern fixed-arity memory-return output unchanged,
  including `wrapper=direct_extern_fixed_arity`, `memory_return`, `memory_home`,
  and `sret_arg` fields.
- Route-debug output stability: route-debug tests keep the `x86 route trace`
  header, function rows, summary ownership/status strings, and the selected
  positive row byte-stable while fallback variants retain the function row and
  omit only the rejected Route 6 row.
- Direct-call/helper-oracle behavior and status labels: helper tests prove the
  public Route 6 record/index APIs return explicit status labels such as
  `Available`, `MissingSourceRelationship`, `MissingSourceProducer`,
  `WrongCall`, `NoMatch`, `MissingArgument`, `DuplicateRelationship`, and
  `AbiBoundExcluded`; x86 route-debug tests prove the direct-call route-debug
  surface consumes only the accepted `ArgumentValue` row.
- Expected-string stability: Step 3 found no expectation rewrites or weakened
  strings in the accepted Step 2 slice; the test assertions still compare exact
  headers, function rows, wrapper lines, and the selected Route 6 row.
- Baseline-stability behavior: existing `test_after.log` records the delegated
  narrow proof passing `backend_prepared_lookup_helper` and
  `backend_x86_route_debug` (`2/2` tests passed) after the accepted Step 2 code
  slice.

## Suggested Next

Execute Step 4 route-quality review. The reviewer should inspect whether the
accepted Step 2 implementation and Step 3 proof matrix still match the active
source idea, especially the recorded limitation that Route 6 owns only the
scalar `ArgumentValue` source-record compatibility while `ConsumedPlans` remains
the prepared fallback/source-id oracle.

## Watchouts

- Step 3 is documentation/proof-record only; no implementation, plan, idea,
  docs, or proof-log files were touched.
- The matrix records existing coverage, not new proof execution. The accepted
  Step 2 proof remains `test_after.log`.
- The reviewer should keep an eye on naming/ownership boundaries: the shared
  BIR check is a Route 6 `ArgumentValue` source-record compatibility predicate,
  not a general semantic identity oracle.

## Proof

Delegated packet was proof-record only. No build or CTest was run because Step 2
already wrote the accepted `test_after.log` and Step 3 did not change code.

Existing Step 2 proof log retained:

```bash
set -o pipefail && cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log
```

Result: `2/2` tests passed: `backend_prepared_lookup_helper` and
`backend_x86_route_debug`.

Step 3 local validation:

```bash
git diff -- todo.md
git diff --check -- todo.md
git status --short --untracked-files=all
```
