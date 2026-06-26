Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Step 3 added focused green fail-closed backend coverage around the current RV64
frame-slot-address call-argument publication fixture. The new cases reject an
ambiguous second candidate store-source publication, a publication store after
the call materialization point, a non-pointer payload source, and a pointer
payload with no prepared source home. Existing adjacent coverage still covers
missing publication facts, duplicate matching facts, mismatched destination
stack offset, malformed source selection, malformed address materialization,
and unsupported destination register shapes.

Accepted-route assertions for the Step 2 value-publication contract are
deferred. The current prepared metadata surface exposes only generic
`store_source_publications.records`; there is not yet a distinct prepared
call-argument value-publication fact tying a specific prior
`StoreLocalPublication` payload source to a specific frame-slot call-argument
object and callsite. Adding an accepted test with only the generic record would
re-bless the route Step 2 rejected as insufficient authority. Likewise, a
storage-address-as-payload fail-closed assertion is blocked until Step 4 adds
that explicit fact schema, because the current helper can still treat another
pointer-valued stack home as a payload source.

## Suggested Next

Execute Step 4 by introducing the explicit prepared call-argument
value-publication fact schema and then implementing the RV64 object route
against that fact. The first Step 4 test should assert that the initialized
`va_list` payload is stored into the argument object before address
materialization, and should add the currently blocked storage-address-as-payload
fail-closed shape once the fact can distinguish payload authority from a generic
pointer home.

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
- Generic `StoreLocalPublication` records are useful input facts but are not the
  Step 392 callsite authority by themselves. Do not extend the current accepted
  test as if that generic record were the final value-publication schema.

## Proof

Delegated proof run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed. Proof log: `test_after.log`.
