# 623 Step 5 Residual Classification Notes

Inputs:

- `build/agent_state/623_step1_cast_residuals.tsv`
- `build/agent_state/623_step1_non_cast_guard_rows.tsv`
- `build/agent_state/623_step2_cast_owner_buckets.tsv`
- `build/agent_state/623_step4_cast_guard_summary.tsv`
- `build/agent_state/623_step4_rerun_logs/`

Output:

- `build/agent_state/623_step5_residual_classification.tsv`

Coverage:

- 18/18 `rv64-consumer:width-preserving-zext-i32-to-i32` residual rows classified.
- 3/3 remaining `rv64-consumer:width-preserving-trunc-i32-to-i32` rows classified.
- `src/pr81556.c` is included as the trunc-family runtime-mismatch row, not as a compile-time CastInst unsupported row.
- `src/p18298.c` is retained only as the passing contrast from Step 4 and is not a residual row.

Classification summary:

- The 18 zext rows still fail with `RV64_C4C_OBJ_COMPILE_FAIL` and a `CastInst` unsupported diagnostic. Step 2 already showed complete producer facts and a present prepared move fact for each row, so the first missing owner remains RV64 object-route `CastInst` consumption for width-preserving zext.
- Two trunc rows, `src/20030714-1.c` and `src/pr81555.c`, still fail with `RV64_C4C_OBJ_COMPILE_FAIL` and a `CastInst` unsupported diagnostic. `src/pr81555.c` now reports a later Step 4 owner (`i32 %t10`) than the original Step 2 saved residual (`i32 %t5`), but the failure stays inside the same width-preserving trunc compile-time owner family.
- `src/pr81556.c` emitted object/binary artifacts and moved to `RV64_BACKEND_RUNTIME_MISMATCH` with `c4c_exit=Subprocess aborted`. Its next owner is runtime correctness after trunc CastInst emission, so it should be kept separate from compile-time CastInst consumer broadening.

Boundary evidence:

- The 60 non-cast guard rows from Step 1 and Step 4 are retained as boundary evidence only. Step 4 kept all 60 failing in non-cast owner classes: 10 `BinaryInst`, 39 `CallInst`, 1 `LoadLocalInst`, 7 `SelectInst`, and 3 `StoreLocalInst`.
- These guard rows should not drive the next cast-consumer implementation packet.

Suggested next implementation candidate:

- Target the `rv64-consumer:width-preserving-zext-i32-to-i32` compile-time owner family first. It covers 18 same-operation rows, all with complete BIR producer/prepared move evidence, and none passed in Step 4.
- Do not use `src/pr81556.c` as the next compile-time consumer candidate; it is already past object emission and needs a runtime-mismatch packet.
