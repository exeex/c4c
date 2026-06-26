Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Pin The Backend Selection Failure

# Current Packet

## Just Finished

Continuation Step 2 pinned the RV64 object-emission selection failure. The
explicit call-argument value-publication route is selected for both `dummy`
calls; it is not skipped, absent, mismatched, or shadowed by the older address
route. The failing branch is inside
`prepared_frame_slot_address_call_argument_publication` in
`src/backend/mir/riscv/codegen/object_emission.cpp`.

The exact bad guard/rewrite is the `plan.source_load_local != nullptr` block:
after `source_value` is initialized from `fact->payload_value`, the helper
rewrites it to `bir::Value::named(plan.source_load_local->result.type,
plan.source_load_local->slot_name)`. For `va-arg-13.c`, the explicit fact
payloads are `%t7.memcpy.copy.0` and `%t14.memcpy.copy.0`, but their
`source_load_local->slot_name` is `%lv.state.8`. `append_rv64_move_value_to_register`
then materializes `%lv.state.8` from `s1`, so the final emitted stores write
the local `va_list` storage address into the argument objects at stack offsets
24 and 32.

## Suggested Next

Execute Step 3 by repairing this narrow load-local payload materialization
rule. The minimal semantic repair target is to preserve the explicit
`fact->payload_value` for call-argument value-publication facts when the
matching store-source record has `source_load_local=yes`, instead of rewriting
to `source_load_local->slot_name`. Add focused coverage for this representative
shape before or with the implementation.

## Watchouts

- Do not hard-code `va-arg-13.c`, `test`, `dummy`, literal registers, stack
  offsets, or the abort branch.
- Do not reopen idea 391 unless evidence shows incoming variadic GPR payloads
  are no longer saved into the backing area.
- Do not treat generic `StoreLocalPublication` records alone as callsite
  authority; Step 392 requires the explicit prepared
  `call_argument_value_publications` fact and its referenced source/destination
  records to agree.
- Keep the argument object address, the local `va_list` storage address, and
  the initialized save-area pointer payload separate in all notes and proof.
- The representative facts are valid enough to identify the callsite/object,
  so do not route this as an absent-fact or mismatched-fact problem. The live
  question is how an explicit load-local payload should be materialized without
  collapsing back to the source storage address.
- Step 2 evidence says the RV64 FrameSlotAddress branch is selected. If
  `prepared_frame_slot_address_call_argument_publication` returned null, the
  prepared call fragment would fail closed; instead the object is emitted with
  stores before `addi a0,sp,24` and `addi a0,sp,32`.
- The trace command produced the current contract-first debug surface only; the
  decisive evidence is the prepared dump plus object-emission branch analysis
  in `build/agent_state/392_cont_step2_backend-selection.analysis.log`.

## Proof

Delegated proof run:
`cmake --build --preset default --target c4cll && mkdir -p build/agent_state && build/c4cll --target riscv64-linux-gnu --trace-mir --mir-focus-function test tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/392_cont_step2_va-arg-13.trace-mir.log 2>&1`.

Result: passed; trace log captured. Analysis log:
`build/agent_state/392_cont_step2_backend-selection.analysis.log`.
