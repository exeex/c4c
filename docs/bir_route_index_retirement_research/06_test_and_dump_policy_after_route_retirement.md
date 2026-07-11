# Test And Dump Policy After Route Retirement

This document defines the test and dump policy for retiring numbered BIR route
APIs. The post-retirement proof model should not keep default intermediate
route dump comparison as the main correctness gate. Route-view tests may stay
only when they prove a named BIR view or a compatibility adapter that is being
actively retired. Correctness for executable behavior should move upward to
prepared contracts, MIR views, object emission, and runtime or object-runtime
checks.

The policy is:

- keep tests that validate durable prepared, MIR, object, or runtime behavior.
- gate route-numbered dump or route-view tests behind a named migration need.
- rewrite route-numbered dump expectations to named prepared or named BIR view
  vocabulary when the surface becomes public.
- delete route-only comparisons once the same fact is proven by named prepared
  authority plus downstream MIR/object/runtime proof.

## Current Test Surface Decisions

| Current surface | Examples | Decision | Post-retirement policy |
| --- | --- | --- | --- |
| Semantic BIR dump tests | `backend_cli_dump_bir_is_semantic`, `backend_cli_dump_bir_vla_goto_stackrestore_cfg`, `backend_cli_dump_bir_layout_sensitive_aggregate`, `backend_cli_dump_bir_00204_stdarg_semantic_handoff` in `tests/backend/bir/CMakeLists.txt`; harness `tests/backend/cmake/run_backend_dump_flag_case.cmake`. | Keep, with route spelling forbidden. | These tests prove `--dump-bir` exposes semantic BIR instead of prepared or route trace output. They should keep forbidding route-summary text and must not become route-index comparison baselines. |
| Prepared BIR dump contract tests | `backend_cli_dump_prepared_bir_is_prepared`, `backend_cli_dump_prepared_bir_exposes_contract_sections`, `backend_cli_dump_prepared_bir_local_arg_call_contract`, and other `--dump-prepared-bir` rows in `tests/backend/bir/CMakeLists.txt`. | Keep and rewrite route-numbered snippets as named prepared facts. | Prepared dumps are the right public diagnostic surface for `PreparedFunctionLookups`, value homes, block-entry publications, move bundles, freshness, call plans, and frame plans. Remove `route4_*`, `route5_status`, and `route5_agrees` snippets when equivalent named prepared proof rows exist. |
| Prepared internal contract tests | `backend_prealloc_prepared_contract_verifier`, `backend_prealloc_block_entry_publications`, `backend_prepare_authoritative_join_ownership`, `backend_riscv_prepared_edge_publication`, `backend_prepared_lookup_helper`, `backend_prepared_object_consumer_contract`. | Keep. | These are durable because they validate prepared authority directly: block-entry publication, edge publication, object traversal, consumer diagnostics, and contract verifier behavior. They should not assert that Route 4 or Route 5 agreement fields are authority. |
| Prepared printer tests | `backend_prepared_printer` plus printer code in `src/backend/prealloc/prepared_printer/value_locations.cpp` and `src/backend/prealloc/prepared_printer/select_chains.cpp`. | Keep, then rewrite. | Keep while dumps are public diagnostics. Rewrite rows that print Route 4 block-entry attribution or Route 5 join-source agreement to named publication-proof vocabulary, or remove them if the same diagnostic is locally recomputable and no longer public. |
| MIR prepared view tests | `backend_prepared_mir_core_view`, `backend_prepared_mir_core_comparator`, `backend_aarch64_prepared_handoff_gate`, and prepared AArch64 record tests in `tests/backend/mir/CMakeLists.txt`. | Keep and promote as gates. | These tests should become the normal proof that prepared data reached MIR with the right block, value-home, publication, freshness, comparison, call, and object facts. They replace route dump comparison for handoff correctness. |
| Object emission tests | `backend_object_model_records`, `backend_riscv_object_emission`, `backend_aarch64_object_emission`, `backend_codegen_object_*`, `backend_obj_runtime_*`, and harnesses such as `run_backend_codegen_object_case.cmake` and `run_backend_rv64_object_runtime_case.cmake`. | Keep and promote as gates. | Object and object-runtime proof is the right end-to-end check for lowered storage, relocation, and executable payload behavior. Route dump agreement is not a substitute for object correctness. |
| Runtime tests | `backend_rv64_runtime_*` via `run_backend_rv64_runtime_case.cmake` and route/runtime paired rows named in baseline triage. | Keep and promote as gates when behavior is executable. | Runtime proof should cover observable program behavior after MIR and object lowering. A passing route-view test alone is insufficient for any capability that has a runtime or object-runtime surface. |
| `backend_codegen_route_*_observe_semantic_bir` tests | Route-style CTest rows built through `c4c_add_backend_codegen_route_test` and `run_backend_codegen_route_case.cmake`, especially rows with `observe_semantic_bir`. | Gate, then rewrite or delete. | These tests may stay temporarily as semantic-BIR observation tests during migration. A new route-view test is justified only if it proves a named BIR view boundary. Once MIR/object/runtime or prepared contract coverage exists, delete the route-only duplicate or rewrite it to named BIR dump terminology. |
| Route 4 block-entry attribution checks | `route4_block_entry_publication_*` fields in `src/backend/prealloc/value_locations.hpp`, attribution in `src/backend/prealloc/prepared_lookups.cpp`, printer proof rows in `value_locations.cpp`. | Gate and rewrite. | Keep only while Phase 1 and Phase 2 migrations need compatibility proof. Rewrite expectations to a named block-entry publication agreement result. Delete route-status expectations after prepared block-entry publication and MIR/object proof cover the behavior. |
| Route 5 join-source agreement checks | `route5_join_source`, `route5_join_source_status`, `route5_join_source_agrees` in `src/backend/prealloc/publication_plans.hpp`; dump text in `select_chains.cpp`; RV64/AArch64 edge-publication diagnostics. | Gate and rewrite. | Keep while Route 5 is the compatibility proof for current-block join-source agreement. Rewrite to named publication view proof when a `BirPublicationView` adapter exists. Delete stored route agreement expectations once prepared edge publication, selected freshness, move bundle, and downstream proof are authoritative. |
| Route 7 comparison validation checks | `Route7IndexReferenceValidation`, facade validation calls in `src/backend/mir/aarch64/codegen/comparison.cpp`, AArch64 comparison tests such as `backend_aarch64_branch_compare_records`, `backend_aarch64_compare_branch_candidate_records`, `backend_aarch64_branch_control_lowering`. | Gate and rewrite. | Route 7 may remain as a named comparison/control-value proof adapter. It must not be used for publication, freshness, stack destination, move execution, or prepared MIR authority. Rewrite tests to assert named comparison proof or prepared branch/comparison MIR records. |

## Proof Hierarchy

Intermediate route dump comparison should be the weakest proof, not the normal
acceptance proof. Use this hierarchy when choosing proof for future
implementation packets:

1. Runtime or object-runtime proof when the behavior has executable output.
2. Object emission proof when correctness depends on object model, relocation,
   section, symbol, or encoded instruction behavior.
3. Target MIR and prepared-MIR proof when correctness depends on target records,
   operand resolution, branch/control lowering, memory operands, call boundary,
   or prepared handoff.
4. Prepared contract and prepared dump proof when correctness depends on
   prepared publication, value homes, freshness, frame, stack, or call facts.
5. Named BIR route-view proof only when the changed surface is a BIR semantic
   view or compatibility proof adapter and no downstream behavior changed.

A route-view test cannot be the only proof for a change that affects MIR,
object, runtime, prepared destination authority, source freshness, move-bundle
execution, stack destination, or call ABI behavior.

## Dump Policy

`--dump-bir` should remain semantic BIR. It should not expose default
intermediate route comparison, prepared sections, route summaries, or route
traces. Existing semantic dump tests should keep forbidding prepared output and
route trace output.

`--dump-prepared-bir` should expose prepared authority and named diagnostic
proof. It may include a temporary compatibility proof row while a route family
is migrating, but the printed public label should move from `route4_*`,
`route5_status`, `route5_agrees`, or Route 7 route-index status to named
concepts:

- block-entry publication agreement for the Route 4 case
- current-block join-source or edge-publication agreement for the Route 5 case
- comparison/control-value agreement for the Route 7 case

The baseline policy should treat route-numbered snippets as transitional. Do
not add new baseline expectations whose only durable assertion is a Route 4,
Route 5, or Route 7 status spelling. If a baseline row currently exists only
because route dump comparison sees a fact, rewrite it to a prepared/MIR/object
or runtime proof before retiring the matching public route API.

## New Route-View Test Justification

A new route-view test is justified only when all of these are true:

- The production code change introduces or migrates a named BIR semantic view,
  such as publication, comparison/control-value, producer, memory access, call
  boundary, or return-chain view.
- The test asserts the named view contract, not `Route 4`, `Route 5`,
  `Route 7`, or other route-numbered spelling as the public behavior.
- No prepared, MIR, object, or runtime surface can observe the contract more
  directly.
- The test includes a planned deletion or rewrite point when all consumers have
  moved off the compatibility adapter.
- The test does not weaken an existing runtime, object, MIR, prepared, or dump
  expectation to make a narrow case pass.

Do not add route-view tests for convenience snapshots, baseline churn, or
testcase-shaped proof. A route-view test that proves only one named failing
case while nearby same-feature behavior remains unexamined is an overfit risk.

## Route-Specific Test Rules

### Route 4

Route 4 tests should focus on named block-entry or current-block publication
agreement. Keep compatibility tests only while
`attribute_route4_block_entry_publication_if_agreeing` and
`find_agreeing_route4_block_entry_publication` are being moved. The durable
tests are prepared block-entry publication, prepared value home, MIR handoff,
object, and runtime tests.

Delete or rewrite Route 4 dump snippets after the prepared printer no longer
stores `route4_block_entry_publication_*` fields as public output. Do not keep
Route 4 status as a baseline gate for destination register, stack slot,
freshness, or move authority.

### Route 5

Route 5 tests should focus on named edge publication and current-block
join-source agreement while Route 5 remains the compatibility proof adapter.
The durable gates are `PreparedEdgePublicationLookups`,
`PreparedMoveBundleLookups`, prepared value homes, selected
`PreparedValueFreshnessAuthority`, `PreparedMirFunctionView` direct-edge
sources, RV64 prepared edge publication emission, object proof, and runtime
proof.

Rewrite `route5_status` and `route5_agrees` dump expectations to named
publication proof rows, then delete them when selected freshness and downstream
MIR/object/runtime proof cover the same behavior. Do not let Route 5 agreement
replace source freshness, destination home validity, stack destination
authority, or move execution proof.

### Route 7

Route 7 tests should stay confined to comparison and control-value proof. A
temporary route-view test is acceptable for a named comparison proof adapter
that forwards to `route7_build_comparison_condition_index`, but target-facing
acceptance should come from prepared branch records, AArch64 comparison record
tests, target MIR lowering tests, object proof, or runtime proof.

Delete or rewrite Route 7 route-index validation expectations when comparison
lowering consumes a named comparison/control-value proof adapter. Route 7 must
never be a test oracle for prepared publication, source freshness, stack
destination, move execution, object storage, or runtime data movement.

## Acceptance Rules For Future Slices

Future implementation packets should choose proof by changed behavior:

- Named wrapper only: compile plus a focused named route-view or BIR test is
  acceptable.
- Prepared publication or printer change: prepared contract/printer tests and
  prepared dump snippets are required.
- MIR handoff change: prepared MIR core/comparator or target MIR tests are
  required.
- Object or encoded target behavior: object emission tests are required.
- Executable behavior: runtime or object-runtime tests are required.

Expectation rewrites are acceptable only when the same semantic contract is
kept under a named prepared, MIR, object, runtime, or named BIR view surface.
Expectation downgrades, unsupported-marker changes, allowlist changes, or
baseline-only acceptance are not route-retirement progress.
