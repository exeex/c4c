Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Representative Publication Path

# Current Packet

## Just Finished

Continuation Step 3 repaired the RV64 frame-slot-address call-argument
publication path for load-local payload producers. In
`prepared_frame_slot_address_call_argument_publication`, a matching
`source_load_local` store-source record now validates that the load-local result
is exactly the explicit `fact->payload_value`, but it no longer rewrites the
payload to `source_load_local->slot_name`. This preserves the prepared
call-argument value-publication payload as the value stored into the argument
object before materializing that object's address.

Focused backend coverage now includes a frame-slot-address call fixture where
the prepared load-local producer's result lives in `s2` while the original
storage slot address lives in `s1`. The test asserts the pre-call stores move
from the load-local payload register, proving the route does not collapse back
to the storage address. The fail-closed matrix now also mutates only the
load-local producer result in that representative fixture while keeping the
fact and store-source payload aligned, proving the explicit
`PreparedCallArgumentValuePublicationFact::payload_value` equality guard is
required. Existing fail-closed variants remain in the backend subset.

## Suggested Next

Run the representative `va-arg-13.c` RV64 object-route case again to confirm the
prior abort boundary is gone or to classify the next concrete boundary without
broadening the plan.

## Watchouts

- The repair keeps the explicit fact as authority; it does not infer payload
  authority from a generic `StoreLocalPublication` alone.
- The `source_load_local` path now fails closed if the producer result is not
  the same pointer value named by the prepared call-argument value-publication
  fact.
- This packet did not rerun the standalone `va-arg-13.c` representative; the
  next packet should preserve a case log if it appears.

## Proof

Delegated proof run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.

Result: passed. Proof log: `test_after.log`.
