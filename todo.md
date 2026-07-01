Status: Active
Source Idea Path: ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace RV64 Fixed-Frame Consumption

# Current Packet

## Just Finished

Step 2 of `plan.md` traced the minimal RV64 object-emission consumer path for
coherent prepared fixed-frame plans without code or test edits. The producer
fact path is:
`src/backend/prealloc/frame.hpp` defines `PreparedStackLayout` with
`frame_size_bytes`, `frame_alignment_bytes`, and `frame_slots`;
`PreparedFrameSlot` owns `slot_id`, `function_name`, `offset_bytes`,
`size_bytes`, `align_bytes`, and `fixed_location`; and
`PreparedFramePlanFunction` republishes per-function `frame_size_bytes`,
`frame_alignment_bytes`, `saved_callee_registers`, `frame_slot_order`,
`has_dynamic_stack`, and `uses_frame_pointer_for_fixed_slots`.
`src/backend/prealloc/frame_plan.cpp:199` `populate_frame_plan` seeds
frame size/alignment from prepared addressing, folds matching
`prepared.stack_layout.frame_slots` into `frame_slot_order`, extends frame
size/alignment from slot extents, records dynamic stack use, assigns
saved-register slots only for non-dynamic frames, sorts/deduplicates
`frame_slot_order`, and sets `uses_frame_pointer_for_fixed_slots` only when a
dynamic frame also has fixed slots. `src/backend/prealloc/stack_layout/lookups.cpp:5`
provides `find_frame_slot_by_id` for prepared slot membership checks.

The RV64 object consumer entry point is
`src/backend/mir/riscv/codegen/object_emission.cpp:10237`
`prepared_function_to_object_function`. It looks up `addressing` with
`prepare::find_prepared_addressing`, looks up `frame_plan` with
`prepare::find_prepared_frame_plan`, then gates object emission through
`rv64_object_stack_frame_size(addressing, frame_plan, prepared.stack_layout)`.
`rv64_object_stack_frame_size` currently combines addressing and prepared frame
sizes, extends by saved-register slot extents, rounds to a 16-byte frame, and
rejects unless the aligned frame fits a signed 12-bit immediate. That helper is
the current fixed-frame validation and large-frame admission boundary.

For Step 3, keep the implementation boundary around
`rv64_object_stack_frame_size`, `make_rv64_stack_frame_prologue_fragment`,
`append_rv64_stack_frame_epilogue`,
`make_rv64_call_frame_prologue_fragment`, and
`append_rv64_call_frame_epilogue`. Split validation from immediate
encodability: accept only coherent fixed prepared frame plans and keep missing
plans, dynamic stack, `uses_frame_pointer_for_fixed_slots`, contradictory
stack-layout vs frame-plan facts, incomplete saved-register placement, size
overflow, unsupported alignment, and non-fixed-frame variants fail-closed.
Reuse `append_rv64_load_immediate` as the existing large-constant helper and add
a narrow stack-pointer adjustment helper for large prepared frame sizes instead
of widening testcase-specific logic. The `has_call` path separately checks
`rv64_call_frame_size` and `rv64_call_frame_ra_offset` for signed 12-bit
encodability; large fixed frames with calls need that boundary handled or left
explicitly fail-closed. The representative has calls.

For Step 4, the fixed-slot addressing boundary is the family of helpers that
currently validate prepared slot facts and then collapse to signed 12-bit
offsets: `prepared_stack_slot_home_offset`,
`prepared_stack_slot_home_offset_for_value`,
`prepared_frame_slot_absolute_offset`,
`fragment_for_prepared_frame_address_materialization`,
`prepared_frame_slot_address_materialization_offset`, and
`append_rv64_materialize_or_move_store_value`. Direct load/store consumers flow
through `fragment_for_prepared_store_local` and
`fragment_for_prepared_load_local`. Low-level encoders such as
`append_rv64_store_register_to_stack`, `append_rv64_load_stack_to_register`,
`append_rv64_store_fpr_to_stack`, and `append_rv64_load_stack_to_fpr` should
remain immediate-offset encoders; large fixed-slot offsets should be handled by
a fact-driven address materialization boundary above them.

Artifacts:
- `build/agent_state/509_step2_fixed_frame_trace/trace_summary.md`
- `build/agent_state/509_step2_fixed_frame_trace/producer_fact_path.rg.txt`
- `build/agent_state/509_step2_fixed_frame_trace/rv64_consumer_path.rg.txt`
- `build/agent_state/509_step2_fixed_frame_trace/prepared_frame_structs.snip.txt`
- `build/agent_state/509_step2_fixed_frame_trace/frame_plan_population.snip.txt`
- `build/agent_state/509_step2_fixed_frame_trace/object_entry_and_frame_gate.snip.txt`
- `build/agent_state/509_step2_fixed_frame_trace/stack_frame_size_gate.snip.txt`
- `build/agent_state/509_step2_fixed_frame_trace/prologue_epilogue_helpers.snip.txt`
- `build/agent_state/509_step2_fixed_frame_trace/encoding_immediate_helpers.snip.txt`
- `build/agent_state/509_step2_fixed_frame_trace/value_home_offset_gate.snip.txt`
- `build/agent_state/509_step2_fixed_frame_trace/frame_slot_memory_offset_gate.snip.txt`
- `build/agent_state/509_step2_fixed_frame_trace/frame_address_materialization_gate.snip.txt`

## Suggested Next

Execute Step 3: implement fixed-frame validation and large stack-pointer
adjustment in `src/backend/mir/riscv/codegen/object_emission.cpp`. Keep the
first code packet narrow: accept coherent fixed prepared frames beyond signed
12-bit frame sizes, materialize large prologue/epilogue stack adjustments from
prepared frame facts, and keep unsupported variants fail-closed before touching
fixed-slot load/store addressing.

## Watchouts

- Keep the route limited to RV64 object emission consuming explicit prepared
  fixed-frame facts.
- Do not infer frame size, stack offsets, widths, alignments, or slot layout
  from testcase names, raw slot counts, source shape, or magic constants.
- Do not include dynamic stack, FPR, F128, vector, varargs, producer-layout, or
  broad ABI support in this plan.
- Preserve the FPR/F128/vector boundary: `diagnose_unsupported_prepared_saved_register_bank`
  must continue rejecting non-GPR saved-register facts. The representative
  prepared plan lists `gpr:s1`, `fpr:fs1`, and `fpr:fs2`, so fixing the generic
  fixed-frame admission gate may expose the existing non-GPR saved-register
  diagnostic next.
- Do not infer frame size, stack offsets, slot widths, slot alignments, or call
  frame shape from `src/20030209-1.c`, `80000`, `10000`, source layout, or raw
  slot counts.
- Step 4 should not weaken low-level load/store immediate encoders. Add or
  reuse a fact-driven address materialization helper above the encoders for
  large fixed-slot offsets.

## Proof

- `scripts/plan_review_state.py set-step --step-id 2 --step-title 'Trace RV64 Fixed-Frame Consumption'`
- `rg` and `sed` traces over `src/backend/prealloc/frame.hpp`,
  `src/backend/prealloc/frame_plan.cpp`,
  `src/backend/prealloc/stack_layout/lookups.cpp`,
  `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`,
  `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`, and
  `src/backend/mir/riscv/codegen/object_emission.cpp` recorded the producer
  and RV64 consumer paths under
  `build/agent_state/509_step2_fixed_frame_trace/`.
- `git diff --check -- todo.md`
- `test_after.log` is not applicable for this trace-only packet; the
  supervisor requested `todo.md` and artifact-local trace proof instead.
