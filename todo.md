Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement First Ordinary ABI Consumer

# Current Packet

## Just Finished

Completed Step 2, "Implement First Ordinary ABI Consumer", lifecycle handoff
for the stack-slot preservation source authority gap around `src/20020529-1.c`.

Current probes show `src/20020529-1.c` still stops at
`unsupported_call_abi` on the first same-module call in `foo`:
`function=foo; block=block_1; block_index=3; instruction_index=0;
callee=f1; args=1; planned_args=1; result=i32 %t0`.

The prepared callsite has three live preserves. `%p.p` and `%p.c` use
callee-saved register preservation and publish complete concrete source and
destination endpoints:

- `%p.p`: `preservation_source=register:a0:value#0`,
  `preservation_destination=register:s1:value#0`
- `%p.c`: `preservation_source=register:a2:value#2`,
  `preservation_destination=register:s2:value#2`

The blocking stack-slot preserve for `%p.b` publishes a concrete destination
slot but not a concrete source register:
`route=stack_slot spill_slot=slot#19+stack24 ... preservation_source=register:value#1
preservation_destination=stack_slot:slot#19:value#1`. The function storage
summary still records `%p.b` in `a1`, but that is not preservation-source
authority for this callsite. Consuming it in RV64 would require inferring the
source register from parameter position or storage state, which is outside
idea 613's consumer-only rule.

No in-scope Step 2 implementation packet was found for this row. The missing
prepared stack-slot preserve source publication was split to
`ideas/open/625_prepared_stack_slot_preservation_source_publication.md`.
The `931004-*` and `931031-1.c` rows remain separate aggregate stack ABI
argument transport work, not stack-slot preservation-source publication.

## Suggested Next

Continue idea 613 with a focused residual refresh for Step 2 rows that already
have complete prepared ABI facts. Keep `src/20000808-1.c` under idea 624 and
`src/20020529-1.c` under idea 625. If the refresh finds no remaining ordinary
call/result Step 2 family with complete prepared facts, advance to Step 3 for
supported stack-frame or return handling with explicit upstream facts.

## Watchouts

- `src/20021219-1.c` is no longer a call ABI row; its next owner is prepared
  join-transfer carrier materialization, not ABI call/result lowering.
- Do not infer stack preservation source registers from ABI parameter position
  or final assembly shape; require explicit prepared endpoints.
- Do not absorb idea 625 producer work back into idea 613; 613 should only
  consume complete preserve-source facts once they exist.
- Keep `931004-*` / `931031-1.c` aggregate stack argument transport, idea-624
  outgoing-stack destination offsets, memory-return/sret, FPR lanes,
  frame-slot publication, stack-frame, return stack-to-register, local/global
  producer, variadic, library, and runtime rows outside this packet unless the
  supervisor changes the boundary.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; `test_after.log` is the preserved
proof log.
