Status: Active
Source Idea Path: ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Step 3 added focused RV64 object-emission coverage around the prepared
frame-slot-address call-argument owner used by the selected `va_list`
payload-copy route.

Coverage added:

- `records_prepared_frame_slot_address_arg_missing_publication_need` asserts
  both frame-slot-address GPR arguments expose
  `FrameSlotAddress` missing-publication needs, preserve source value identity,
  and do not claim local aggregate payload authority from address
  materialization alone.
- `rejects_prepared_frame_slot_address_arg_call_fail_closed_shapes` now also
  rejects duplicate exact frame-slot address materialization facts and
  non-frame-slot materialization kinds in the prepared RV64 object route.

Accepted-route assertion intentionally deferred:

- A green assertion that copies an initialized source `va_list` payload into a
  frame-slot-address argument object before `addi aN, sp, <offset>` requires the
  Step 4 implementation and/or a new explicit prepared publication-copy fact.
  The current public prepared facts expose the missing-publication need but do
  not yet encode the source `va_list` payload-to-destination-object copy edge.

## Suggested Next

Execute Step 4: add the narrow RV64 prepared-call payload-copy implementation
or first expose the missing explicit prepared publication-copy fact if the
implementation cannot connect initialized `va_list` source storage to the
frame-slot-address argument object through existing metadata.

## Watchouts

- Keep idea 389 `va_start` destination-address materialization closed.
- Keep idea 388 `va_end`, idea 386 ordinary frame-slot-address GPR call
  arguments, and idea 387 sret out of scope.
- Do not hard-code `va-arg-13.c`, `test`, `dummy`, registers, stack offsets, or the abort branch.
- Do not broaden this packet into a full `va_arg`, aggregate, or call ABI rewrite.
- Address materialization alone is not proof that the initialized `va_list`
  payload reached `dummy`; the implementation must require a source-to-object
  publication/copy fact and fail closed when the fact is absent or ambiguous.
- The new green coverage intentionally does not assert the accepted copy
  sequence yet because the route cannot pass before Step 4 without either
  weakening expectations or implementing behavior.

## Proof

Command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed.

Log path: `test_after.log`.
