Status: Active
Source Idea Path: ideas/open/623_rv64_cast_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Classify Step 4 no-breadth residuals

# Current Packet

## Just Finished

Step 5 classification is complete. The residual table is
`build/agent_state/623_step5_residual_classification.tsv`, with supporting
notes in `build/agent_state/623_step5_residual_classification_notes.md`.

Coverage recorded:

- 18/18 `rv64-consumer:width-preserving-zext-i32-to-i32` rows remain
  compile-time `RV64_C4C_OBJ_COMPILE_FAIL` / `CastInst` unsupported residuals.
- 3/3 remaining `rv64-consumer:width-preserving-trunc-i32-to-i32` rows are
  classified: `src/20030714-1.c` and `src/pr81555.c` remain compile-time
  `CastInst` unsupported residuals; `src/pr81556.c` has moved to
  `RV64_BACKEND_RUNTIME_MISMATCH` after object/binary emission.
- `src/p18298.c` is retained only as the Step 4 passing trunc contrast.
- The 60 non-cast guard rows are explicitly retained as boundary evidence only:
  10 `BinaryInst`, 39 `CallInst`, 1 `LoadLocalInst`, 7 `SelectInst`, and 3
  `StoreLocalInst` rows all remained failed in non-cast owner classes.

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

No build proof was required for this classification-only Step 5 packet.
Classification artifacts were generated from existing Step 1, Step 2, and Step
4 artifacts. No `test_after.log` was produced because the delegated proof
explicitly required no build proof.
