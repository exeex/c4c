Status: Active
Source Idea Path: ideas/open/623_rv64_cast_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Choose the next route from classification

# Current Packet

## Just Finished

Step 6 route selection is complete from the committed Step 5 classification.
The selected next route is the shared 18-row
`rv64-consumer:width-preserving-zext-i32-to-i32` compile-time `CastInst`
consumer family recorded in
`build/agent_state/623_step5_residual_classification.tsv` and summarized in
`build/agent_state/623_step5_residual_classification_notes.md`.

Selection basis:

- All 18 zext rows remain compile-time `RV64_C4C_OBJ_COMPILE_FAIL` /
  `CastInst` unsupported residuals.
- The 18-row zext family has complete BIR producer/prepared move evidence and
  no Step 4 pass-through row, making it the broadest coherent next consumer
  packet.
- Trunc rows are intentionally separate: `src/20030714-1.c` and
  `src/pr81555.c` remain compile-time trunc `CastInst` residuals, while
  `src/pr81556.c` is a runtime-mismatch row after object/binary emission.
- The 60 non-cast guard rows remain boundary evidence only and do not drive the
  next implementation packet.

## Suggested Next

Execute a focused implementation packet for
`rv64-consumer:width-preserving-zext-i32-to-i32` compile-time `CastInst`
consumption. Use the Step 5 table as the row authority and prove against the
18 zext residuals before considering trunc or runtime-mismatch work.

## Watchouts

- Do not use `src/pr81556.c` as the next compile-time CastInst consumer
  candidate; it is past object emission and needs a runtime-mismatch packet.
- `src/pr81555.c` is still trunc-family, but Step 4 now reports a later owner
  (`i32 %t10`) than the Step 2 saved residual (`i32 %t5`).
- Keep the 60 non-cast guard rows as boundary evidence only.
- Do not use expectation, unsupported-marker, allowlist, timeout, accounting,
  or named-case-only changes as progress.

## Proof

No build proof was required for this Step 6 route-selection packet. The route
was selected by inspecting the committed Step 5 classification artifacts. No
`test_after.log` was produced because the delegated proof explicitly required
no build proof.
