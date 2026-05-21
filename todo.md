Status: Active
Source Idea Path: ideas/open/379_aarch64_local_aggregate_copy_load_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Publication

# Current Packet

## Just Finished

Step 3 repaired the AArch64 local aggregate address publication boundary for
stack-homed local-slot pointer address values. When prepared BIR contains a
pointer add/sub whose base is a prepared `local_slot` object, lowering now
publishes `sp + local-slot-offset` directly and stores that published address
to any stack home for the pointer value before later pointer stores or
pointer-derived aggregate copies consume it.

The Step 2 focused case now emits the real local frame address for `&local`:
`add x9, sp, #0` followed by stack-home/pointer stores, instead of reloading
the aggregate base from the `%lv.local.0` spill slot (`ldr x9, [sp, #72]`).
The existing focused route assertion remained valid because the general repair
materialized the same frame-address sequence in the same prepared register.

## Suggested Next

Delegate Step 4 to prove `00216` advances past the localized `foo` crash and
classify any new first bad fact without expanding this owner silently.

## Watchouts

The repair is keyed to prepared local-slot metadata and pointer add/sub
publication, not to `00216`, the focused test filename, stack offsets,
registers, or a nearby `ldrb` sequence. Step 4 should check whether the
external `00216` failure advances; any relocation, function-pointer-table, or
timeout owner should be handed back to supervisor routing.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: build completed and the backend subset passed 146/146 tests, including
`backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address`.
Proof log: `test_after.log`.
