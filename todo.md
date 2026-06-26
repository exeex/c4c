Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Value-Publication Route

# Current Packet

## Just Finished

Step 2 classified the route as a guarded RV64 prepared/object call-argument
value publication for frame-slot address arguments. The implementation owner is
the existing prepared-call object fragment path around
`fragment_for_prepared_call` and
`prepared_frame_slot_address_call_argument_publication`, but the route must be
backed by an explicit prepared value-publication fact rather than by
`missing_frame_slot_arg_publication=yes` alone.

Selected route: when a call argument is a frame-slot object address whose object
was populated by a prior `StoreLocalPublication` from an initialized `va_list`
payload, RV64 object emission should first copy the initialized payload value
into the destination argument object, then materialize the argument object's
address into the ABI register. For the current representative, the required
edges are `%t7.memcpy.copy.0` value id 14 -> `%t7` value id 15 before call inst
9 and `%t14.memcpy.copy.0` value id 18 -> `%t14` value id 19 before call inst
16. The payload is the loaded value of `%lv.state.8`, not the address of
`%lv.state.8` storage.

Required guards for the accepted route:

- The call argument has `source_encoding=frame_slot`, destination GPR bank,
  contiguous width 1, pointer-sized source size 8, and
  `arg.source_selection=frame_slot_address`.
- `find_prepared_missing_frame_slot_call_argument_publication_need` is
  available for that same argument, has kind `FrameSlotAddress`, the same
  `source_selection` and `source_value_id`, `source_materializes_address=yes`,
  and `may_emit_local_aggregate_address_payload=no`.
- A distinct prepared call-argument value-publication fact exists for the same
  function, block, and call argument. It links exactly one earlier
  `StoreLocalPublication` record to the argument object by destination frame
  slot id, destination stack offset, destination size 8, destination object when
  available, and the call argument source value id/name.
- The publication source is a pointer payload with a resolvable prepared home.
  For the `va_list` expression case it should identify the store source value
  (`%t7.memcpy.copy.0` / `%t14.memcpy.copy.0`) and, when produced by
  `load_local`, retain the loaded local source (`%lv.state.8`) as provenance so
  object emission copies the initialized value payload instead of the local
  storage address.
- The publication store instruction is in the same block before the call
  materialization instruction, and the selected frame-slot address offset is
  independently valid through the existing stack-layout/address-materialization
  checks.

Fail-closed cases belong to the RV64 prepared-call/object admission path: absent
value-publication fact, duplicate matching facts, multiple possible earlier
stores, mismatched function/block/call argument, mismatched destination slot or
stack offset, non-pointer or non-8-byte source payload, missing source home,
store after the call, malformed address materialization, or a fact that names
the `va_list` storage address as payload authority. These should reject the RV64
prepared call fragment instead of silently emitting an address payload.

This does not reopen prior ideas. Idea 386 owns ordinary frame-slot-address GPR
call argument lowering; this route is specifically the value copied into the
addressed argument object. Idea 387 owns memory-return/sret calls, not this
same-module pointer argument. Idea 388 owns `va_end`, which is not part of the
failing call. Idea 389 owns `va_start` destination-address materialization, which
already writes the local `va_list` object. Idea 390 owns the prepared
frame-slot-address call-argument publication shape; this route adds a stricter
initialized-value edge for `va_list` expression payloads. Idea 391 owns incoming
variadic GPR save-area publication, and Step 1 evidence shows that publication
is present.

## Suggested Next

Execute Step 3: add focused RV64 backend coverage for the accepted
value-publication route and fail-closed variants. The accepted case should prove
the emitted fragment stores the initialized pointer payload into the argument
object before materializing the object's address into the ABI register. Negative
cases should cover missing, duplicate, ambiguous, mismatched destination,
mismatched source, and malformed address-materialization facts.

## Watchouts

- Do not solve this by hard-coding `va-arg-13.c`, `test`, `dummy`, offset
  `0x80`, offset `0x48`, or the abort branch.
- Do not reopen idea 391 unless evidence shows incoming variadic GPR payloads
  are no longer stored into the backing save area.
- Do not treat `missing_frame_slot_arg_publication=yes` or a generic
  `StoreLocalPublication` record alone as enough authority. Step 3/4 need an
  explicit prepared fact tying a specific payload source store to a specific
  frame-slot call-argument object and callsite.
- Preserve the distinction between the argument object's address, the local
  `va_list` storage address, and the initialized save-area pointer payload.
- The likely implementation can extend the existing frame-slot-address
  publication helper, but accepted/fail-closed coverage should pin the new
  value-publication fact so the route is not inferred from testcase shape.

## Proof

Todo-only classification; no implementation files or tests were changed.
Delegated proof run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed. Proof log: `test_after.log`.
