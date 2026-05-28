Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Inventory The Separable Dispatch Seams

# Current Packet

## Just Finished

Completed Step 2 - Inventory The Separable Dispatch Seams for the active
decomposition route in
`ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md`.

Inventory from existing evidence only:

| Seam | Likely owner surface | Status | Evidence source |
| --- | --- | --- | --- |
| Store-global publication ownership/accounting for stack-homed selected values | `src/backend/mir/aarch64/codegen/memory.cpp`, `memory.hpp`, and dispatch accounting at `dispatch.cpp` unsupported-diagnostic suppression sites | Partial dirty implementation exists; not accepted recovery progress | Dirty diff adds `lower_pending_store_global_stack_value_publications`, `lower_published_store_global_stack_value_publication`, and dispatch use of `published_store_global_stack_values`; review records this as semantic but route-widening evidence |
| Store-local selected publication ownership/accounting | `memory.cpp`/`memory.hpp` helper around `future_store_local_stack_value_publication_covers_instruction`, with dispatch as the current consumer | Partial dirty implementation exists, but highest drift risk because ownership is inferred from a later same-block store-local consumer | Dirty diff adds the future-consumer scan; review flags this as the closest route-drift risk and asks for focused nearby coverage or a clearer owner |
| Fused-compare selected operand materialization/order | `dispatch_publication.cpp` and `dispatch_publication.hpp`, especially `lower_missing_fused_compare_operand_publication(s)` and prepared source-producer scratch selection | Partial dirty implementation exists; needs focused coverage before acceptance | Dirty diff adds preferred scratch selection for `SelectMaterialization`; review says this is plausible but introduced through a compare-order assertion |
| Call/outgoing stack argument materialization | `calls.cpp`, especially immediate call-plan lookup and frame-slot call-boundary move lookup | Partial dirty implementation exists; currently separate from latest blocker | Dirty diff adds fallback lookup in `find_immediate_argument_in_call_plan` and prepared call-boundary move classification; prior executor notes say it fixed immediate stack call-argument and aggregate frame-slot assertions |
| Direct edge `LoadLocal` publication source-memory consumption | `dispatch_edge_copies.cpp`, especially `emit_edge_load_local_to_register` and prepared publication/source-memory fallback ordering | Partial dirty implementation exists; previous blocker cleared, not current first bad assertion | Dirty diff makes matching prepared `LoadLocal` edge publications require prepared source-memory authority before falling back to source register/home paths; prior executor note says it reached the fail-closed missing-source-memory case |
| GOT-required `LoadGlobal` materialization | likely `dispatch_value_materialization.cpp` around `emit_prepared_global_load_to_register`, with related selected-node support in `globals.cpp`/`globals.hpp` via `make_load_global_got_materialization_instruction` and dispatch route use in `dispatch.cpp` | Next-blocker candidate; no dirty implementation yet in this packet stack | `test_after.log` first bad assertion is `expected GOT-required global load to use GOT page/low12 materialization`; targeted search finds the assertion in `backend_aarch64_instruction_dispatch_test.cpp` and existing GOT materialization helpers/tests around prepared address materialization |
| `00196` runtime mismatch | remains outside this decomposition inventory unless a focused probe proves shared dispatch/prepared-publication ownership; likely still idea 58's later `78730af2f` family | Separate; do not fold into Step 3 seam selection from current evidence | `test_after.log` reports only the known runtime mismatch for `c_testsuite_aarch64_backend_src_00196_c`; idea 60 and `plan.md` both instruct keeping it separate |

Current proof context remains `2/4` passing in `test_after.log`:
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor` and
`c_testsuite_aarch64_backend_src_00207_c` pass; `backend_aarch64_instruction_dispatch`
fails at GOT-required global-load materialization; `00196` fails with the known
runtime mismatch.

## Suggested Next

Execute Step 3 - Split The Monolithic Probe Into Focused Cases.

Suggested Step 3 focused-probe selection should start with the current
next-blocker candidate, GOT-required `LoadGlobal` materialization, because it is
visible in `test_after.log` and has no partial dirty implementation yet. Prefer
an existing focused backend/MIR route if one already isolates prepared
GOT-required `LoadGlobal` materialization; otherwise propose
`tests/backend/case/aarch64_got_load_global_prepared_memory.c` as the focused
case candidate and bind it to the global-load materialization owner before any
code edits.

## Watchouts

- Do not resume implementation from the next monolithic
  `backend_aarch64_instruction_dispatch` assertion until Step 3/4 bind the
  selected seam to a focused probe and owner.
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

## Proof

No fresh test proof required or run for this documentation-only Step 2 packet.

Evidence used:

- Existing `test_after.log`, showing the current four-test focused proof at
  `2/4` passing and the latest dispatch assertion.
- Existing `review/step3_dispatch_route_review.md`, showing route drift without
  blocking testcase-overfit signals.
- Current dirty `git diff`, showing the partial implementation surfaces and
  helper names.
- Targeted `rg` search for GOT-required `LoadGlobal` materialization owner
  surfaces and existing evidence.
- Active `plan.md` and source idea 60, confirming Step 2's seam-inventory
  requirements.

Proof log path: `test_after.log`.
