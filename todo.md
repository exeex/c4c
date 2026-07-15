# Current Packet

Status: Active
Source Idea Path: ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the bounded route and return to 810

## Just Finished

796 Step 2 converted the selected scalar `StmtEmitter::coerce` residual cast
producer seam to allocate a native `LirValueId`, use native `LirTypeRef`
endpoints, and opt into `requires_native_result_authority`. Vector and
aggregate compatibility producers remain fail-closed. Nearby complex-coercion
coverage asserts both selected FPExt component results and verifies missing,
invalid, duplicate, and foreign result IDs are rejected.

## Suggested Next

Supervisor: select the next 796 packet from the remaining residual inventory;
do not widen this completed cast-producer packet into preserved 801/806 work.

## Watchouts

The selected admission is scalar-only because those cast kinds have complete
native endpoint facts. Keep vector and aggregate compatibility producers
fail-closed; do not parse `%t` text or weaken the cast verifier. Preserve the
dirty 801 aggregate/direct-complex hunks, 810/795 accepted GEP/parameter work,
and the 806 PHI route.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_lir_call_type_ref$'`. The delegated proof
was sufficient for this packet. Per supervisor direction, no canonical root
regression log was created or overwritten.
