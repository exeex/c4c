Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Split The Monolithic Probe Into Focused Cases

# Current Packet

## Just Finished

Completed Step 3 - Split The Monolithic Probe Into Focused Cases for the active
decomposition route in
`ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md`.

Selected or proposed focused probes from existing evidence only:

| Seam | Selected/proposed probe | Existing reuse | Harness gap |
| --- | --- | --- | --- |
| GOT-required `LoadGlobal` materialization | Select next: propose `tests/backend/case/aarch64_got_load_global_prepared_memory.c`, registered as `backend_codegen_route_aarch64_got_load_global_prepared_memory`, requiring `:got:external_data_symbol`, `:got_lo12:external_data_symbol`, and the final load through the materialized GOT entry, while forbidding direct `:lo12:external_data_symbol`. | Existing `backend_aarch64_instruction_dispatch` subtests `load_global_call_argument_uses_got_for_got_required_global`, `stack_preserved_loaded_global_pointer_publishes_before_call_argument_reload`, and `load_global_call_argument_keeps_direct_for_direct_global` isolate the contract at C++ route level, but remain inside the monolithic binary. Existing C cases such as `global_load.c` and `extern_global_array.c` are too generic and have no AArch64 GOT-required route binding. | The backend route harness checks required/forbidden substrings but not instruction ordering. If order or prepared-policy provenance matters, extract the existing C++ subtests into a focused `backend_aarch64_*` test or add an ordering-capable route harness before accepting this as the only proof. |
| Store-global publication ownership/accounting for stack-homed selected values | Propose `tests/backend/case/aarch64_store_global_stack_publication.c`, registered as a backend route test requiring the selected stack publication, global-address materialization, and global store consumer snippets. | Existing monolithic C++ subtests `block_dispatch_publishes_f64_select_before_global_store` and `block_dispatch_publishes_stack_homed_i32_select_before_global_store` already isolate the route-level behavior, but no existing `tests/backend/case/` file creates this selected stack-home global-store publication. Existing `global_store.c` and defined/global store cases are not enough because they do not force selected stack-homed source publication. | Substring route tests cannot prove publication-before-consumer ordering or store-owned accounting. Acceptance should pair any C route case with an extracted focused C++ dispatch test unless the route harness gains ordered snippets. |
| Store-local selected publication ownership/accounting | Propose `tests/backend/case/aarch64_store_local_selected_publication.c`, registered as a backend route test requiring selected publication before local-store writeback/reload snippets. | Existing monolithic C++ subtests `block_dispatch_publishes_stack_homed_global_load_select_before_local_store` and `selected_local_store_materializes_binary_snapshot_value_before_writeback` are the closest reusable probes. Existing backend cases around local stores do not isolate selected publication ownership. | Highest harness gap: the dirty implementation uses a future-consumer scan, and route substrings cannot prove the publication owner is explicit rather than incidental. This seam should use a focused C++ extraction or a richer route harness before accepting the dirty helper as progress. |
| Fused-compare selected operand materialization/order | Propose `tests/backend/case/aarch64_fused_compare_selected_operand_order.c`, registered as a backend route test requiring the selected materialization label/path and compare/branch snippets. | Existing monolithic C++ subtest `selected_global_load_materializes_before_fused_compare_branch` isolates the intended compare-order contract. Existing branch route cases such as `branch_if_lt.c` do not force selected global-load materialization. | The current route harness cannot assert relative order or "no stale reload before compare" except by broad forbidden snippets. Prefer extracting the existing C++ subtest or adding ordered snippets if this seam becomes the selected implementation packet. |
| Call/outgoing stack argument materialization | Reuse or extend existing `tests/backend/case/aarch64_prepared_call_boundary_scalability.c` only for broad call-boundary pressure; propose a narrower `tests/backend/case/aarch64_call_prepared_stack_arguments.c` for immediate stack arguments and aggregate frame-slot call arguments. | Existing monolithic C++ subtests `prepared_immediate_stack_call_argument_lowers_before_direct_call` and `missing_local_aggregate_frame_slot_call_argument_materializes_object_address` isolate the dirty fixes. Existing backend route tests `backend_codegen_route_aarch64_prepared_call_boundary_scalability` and the two AArch64 ALU unpublished load-local cases are related but do not prove these exact outgoing-stack contracts. | Backend route cases can prove emitted call/argument snippets, but they cannot inspect call-plan lookup source or call-boundary move classification. If the next owner is `calls.cpp`, bind proof to focused C++ subtests or add route snippets only as integration evidence. |
| Direct edge `LoadLocal` publication source-memory consumption | Propose `tests/backend/case/aarch64_edge_load_local_prepared_memory.c` only as an integration candidate; primary focused proof should be an extracted C++ route test from `predecessor_join_load_source_publication_uses_prepared_source_memory`. | Existing monolithic C++ subtest `predecessor_join_load_source_publication_uses_prepared_source_memory` already contains both consume-prepared-source-memory and fail-closed missing-source-memory checks. No existing backend case under `tests/backend/case/` can directly force the prepared source-memory authority fact. | Strong harness gap: a C route case can show final load/store assembly but cannot prove source-memory authority was consumed or fail-closed when absent. Do not accept route-only proof for this seam. |
| `00196` runtime mismatch | No Step 3 dispatch/prepared-publication probe selected. Keep `c_testsuite_aarch64_backend_src_00196_c` as the existing integration/runtime failure for idea 58's later `78730af2f` family unless new evidence links it to one seam above. | Existing c-testsuite case remains the proof artifact. No new `tests/backend/case/` candidate should be created from current evidence. | Runtime mismatch evidence does not identify a dispatch prepared-publication seam. Folding it into this decomposition would widen the route. |

Current selection: start Step 4 with the GOT-required `LoadGlobal` seam because
`test_after.log` reports it as the current first bad assertion, it has no dirty
implementation yet, and it can be bound to a narrow owner around prepared global
load materialization before touching code.

## Suggested Next

Execute Step 4 - Bind Each Focused Probe To One Backend Owner.

Suggested Step 4 owner/proof binding should choose the GOT-required
`LoadGlobal` probe first:

- selected probe: `tests/backend/case/aarch64_got_load_global_prepared_memory.c`
  plus the existing C++ route subtests as focused extraction candidates
- likely owner: `dispatch_value_materialization.cpp` around
  `emit_prepared_global_load_to_register`, with related GOT helper support in
  `globals.cpp`/`globals.hpp` and dispatch use sites only as consumers
- narrow proof candidate: build plus
  `ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_aarch64_got_load_global_prepared_memory|backend_aarch64_instruction_dispatch'`
  after the focused test exists; if no route test is created, use a focused
  extracted C++ test name instead of the whole monolithic dispatch binary

## Watchouts

- Do not resume implementation from the next monolithic
  `backend_aarch64_instruction_dispatch` assertion until Step 4 binds the
  selected GOT-required `LoadGlobal` seam to owner files and proof.
- The dirty stack is intentionally preserved. Do not revert it as part of
  decomposition documentation, but do not classify it as accepted progress yet.
- `review/step3_dispatch_route_review.md` described the dirty route as
  non-overfit but drifting; that is a warning, not acceptance.
- Store-local selected publication is the highest drift-risk dirty seam because
  the current evidence is a future-consumer accounting scan. A focused probe
  should prove it before that part is accepted.
- `00196` remains separate. Current evidence shows a c-testsuite runtime
  mismatch, not a dispatch/prepared-publication owner surface.
- Do not weaken tests, add testcase-name checks, or claim todo-only
  classification as backend capability progress.
- New backend route cases under `tests/backend/case/` need matching CMake
  registrations before they are runnable; this packet only selected/proposed
  probes and did not create tests.

## Proof

No fresh test proof required or run for this documentation-only Step 3 packet.

Evidence used:

- Existing `test_after.log`, showing the current four-test focused proof at
  `2/4` passing and the latest dispatch assertion.
- Existing `review/step3_dispatch_route_review.md`, showing route drift without
  blocking testcase-overfit signals.
- Current dirty `git diff`, showing the partial implementation surfaces and
  helper names.
- Targeted `rg` and CTest inventory for existing AArch64 backend route cases,
  backend case files, and monolithic C++ dispatch subtests.
- `tests/backend/cmake/run_backend_codegen_route_case.cmake`, confirming that
  route tests support required/forbidden substrings but not ordering or
  prepared-authority inspection.
- Active `plan.md` and source idea 60, confirming Step 3's focused-probe
  requirements.

Proof log path: `test_after.log`.
