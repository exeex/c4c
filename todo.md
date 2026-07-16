Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.50
Current Step Title: Receive the one 863-authorized direct floating call-result authority row

# Current Packet

## Just Finished

Completed `plan.md` Step 7.50 receiver work: typed Raw BIR now receives the
closed-863 direct zero-argument scalar floating call-result authority row,
retains the result/owner/callee/return-type/role tuple, and requires the exact
downstream floating binary LHS consumer before publication.

## Suggested Next

Supervisor should review and decide the next 734 packet; Step 7.50 is locally
complete and proved with the delegated focused backend interface test.

## Watchouts

Producer-side LIR authority was not edited. `builder.hpp` carries the required
`CallSpec` field for the new Raw-BIR call authority even though the delegated
owned list omitted that header; it was already part of the in-progress local
surface before this packet.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
