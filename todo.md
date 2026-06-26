Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Narrow Va List Value Publication

# Current Packet

## Just Finished

Step 4 implemented the narrow RV64 `va_list` expression call-argument
value-publication route. Prepared metadata now has explicit
`call_argument_value_publications` facts tying a prior
`StoreLocalPublication` payload source to one frame-slot call-argument object
and callsite; the prepared printer emits the fact section. RV64 object emission
now requires one matching explicit fact plus one matching underlying
`StoreLocalPublication` record before it stores the initialized pointer payload
into the argument object and then materializes that object's address.

Focused backend coverage now proves the accepted route and fail-closed shapes
for missing facts, duplicate/ambiguous facts, mismatched destination/object
metadata, missing referenced store source, non-pointer payloads, payloads with
no prepared home, and storage-address-as-payload attempts.

## Suggested Next

Execute Step 5 by rerunning the `va-arg-13.c` RV64 object-route
representative. Record whether the representative advances past the current
abort, and if it exposes a later boundary, keep that boundary separate instead
of expanding this plan.

## Watchouts

- Do not solve any remaining representative failure by hard-coding
  `va-arg-13.c`, `test`, `dummy`, offset `0x80`, offset `0x48`, or the abort
  branch.
- Do not reopen idea 391 unless evidence shows incoming variadic GPR payloads
  are no longer stored into the backing save area.
- Do not treat `missing_frame_slot_arg_publication=yes` or a generic
  `StoreLocalPublication` record alone as enough authority. The implemented
  Step 4 route requires the explicit prepared call-argument value-publication
  fact and its referenced store-source record to agree.
- Preserve the distinction between the argument object's address, the local
  `va_list` storage address, and the initialized save-area pointer payload.
- Generic `StoreLocalPublication` records are useful input facts but are not
  Step 392 callsite authority by themselves. Keep later work on the explicit
  prepared fact surface.

## Proof

Delegated proof run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed, 326/326 backend tests. Proof log: `test_after.log`.
