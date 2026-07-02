Status: Active
Source Idea Path: ideas/open/425_scalar_fpr_salvage_from_try_gcc_torture.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Candidate Impact And Rewrite Risk

# Current Packet

## Just Finished

Step 2 - Classify Candidate Impact And Rewrite Risk ranked the scalar FPR
salvage candidates from the Step 1 evidence and separated accepted candidates
from quarantined `try_gcc_torture` material.

Current evidence basis:

- Fresh 2026-07-01 whole scan in
  `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`:
  `1467` total, `314` pass, `1153` fail. Current scalar-FP-specific bucket is
  `unsupported_floating_cast` with `3` rows.
- Fresh row artifacts:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`,
  `build/agent_state/rv64_gcc_c_torture_backend_failed.full.txt`, and
  per-case logs under `build/rv64_gcc_c_torture_backend/`.
- Older 2026-06-30 instruction-fragment evidence in
  `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`,
  `build/agent_state/unsupported_instruction_fragment_rows.tsv`, and
  `build/agent_state/421_step2_prepared/`: `190`
  `unsupported_instruction_fragment` rows, with a separate
  `Scalar F32/F64 conversion/op residual` bucket of `2` rows.
- `try_gcc_torture` evidence range:
  `0d90e1b6b..try_gcc_torture` (`552` branch-only commits). These commits are
  evidence only; they are not cherry-pick candidates.

Ranked candidate classification:

| Rank | Candidate | Current bucket impact | Representative rows | Commit evidence | Required prepared facts | Implementation surface | Rewrite risk | Decision |
| ---: | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Residual scalar F32/F64 cast support beyond existing object-route admission | Fresh `unsupported_floating_cast` bucket: `3` rows. Keep separate from older instruction-fragment residuals. | `src/cvt-1.c`, `src/920618-1.c`, `src/pr66233.c`; each log reports the object route only supports prepared FPR width casts and I32/I64-to-F32/F64 integer-to-floating casts. | `9d0f64883`, `b0700c2c3`, `88076c4ec`, `18c41c9c3`, `92d77cf2c`, `8bbaf8eb7`, `f95ec37b9`. | Prepared cast opcode, source/result scalar type, source/destination FPR/GPR bank facts, materializable immediates or coherent homes, and a rejection path for unsupported FP/INT directions. | RV64 prepared floating-cast object lowering plus focused diagnostics. | Moderate: small current row count but clean owner; risk is accidentally reopening broad conversion/F128/helper work or treating `conversion.c` as proof. | Accept as the highest-ranked scalar-FPR salvage candidate, but scope narrowly to the three current rows' cast family and require semantic cast coverage, not filename proof. |
| 2 | Older scalar F32/F64 instruction-fragment residual | Older classified instruction-fragment residual: `2` rows. Keep separate from fresh `unsupported_floating_cast`. | `src/ieee/930529-1.c`, `src/ieee/pr72824.c`. Prepared dumps show `double` division plus select/local-byte checks for `930529-1`, and `float` local storage, `fptrunc`, `llvm.copysign.f32`, and `ne float` branch for `pr72824`. | Binary/compare/select evidence from `3b8c6691e`, `c77ae7934`, `c7f003c85`, `54b013526`, `f4d46728e`, `9162d7057`, `c43ad51d2`, `a3b9bbd66`, `500bcaac6`. | Complete prepared BIR for FP binary/call/select/branch, FPR source/result homes, compare predicate facts, and local object facts when local bytes or address-exposed float slots are involved. | Mixed RV64 object lowering: scalar FPR binary/compare, FP branch guard, select/materialization, and possibly local-memory interaction. | High: only two old rows and both are composite; easy to overfit a named IEEE case or mix local-byte/select work into scalar FPR lowering. | Partial salvage only. Use as supporting evidence for future focused ideas after current `unsupported_floating_cast`, not as the first implementation route. |
| 3 | Floating branch guard and compare publication | No distinct fresh bucket; appears inside the older `2` residual rows and branch-shaped prepared dumps. | `src/ieee/pr72824.c` has `ne float %t6, %t7` as a branch condition. | `f4d46728e`, `9162d7057`, `c43ad51d2`, `a3b9bbd66`, `500bcaac6`, plus reload commits `076a9e432`, `35d73dfdc`, `56fc23309`. | Fused FP compare predicate, branch labels, FPR reload/home facts, result publication facts, and a clear owner for branch-owned scalar compare handoff. | RV64 terminator/object branch lowering around scalar FPR compare guards. | Moderate-high: useful ordinary-C surface, but current row count is not independently measured and branch work can drift into broad control-flow rewrites. | Defer as a candidate sub-idea unless Step 3 can bind it to non-F128 rows beyond `pr72824`. |
| 4 | Floating select/materialization | No fresh scalar-FP bucket; only older residual support and broader non-FP select evidence elsewhere. | `src/ieee/930529-1.c` has many integer selects around byte checks after a double operation; not a clean scalar FPR select row. | `c7f003c85`, `54b013526`, `21d6f507c`, `13f54dcb1`. | Prepared select/join-transfer records, source value type, branch-pair ownership, source/destination homes, and materializable true/false arms. | RV64 select/join materialization, possibly independent of scalar FPR. | High for this scalar-FPR runbook: broad select work has larger non-FP owners and the scalar-FP evidence is indirect. | Reject as a scalar-FPR follow-up from idea 425. Route any select work through the existing broader select/join backlog, not this scalar salvage plan. |
| 5 | FPR local-store/reload publication | No current scalar-FP bucket; mostly support machinery adjacent to older residuals. | `src/ieee/pr72824.c` has address-exposed and home-slot `float` locals; current local-memory failures such as `src/bf64-1.c` are not scalar FPR owned. | `0b6b06466`, `017890e27`, `076a9e432`, `35d73dfdc`, `56fc23309`. | Prepared local object records, FPR home/reload facts, store-source publication, stack slot layout, and no aggregate/byval ownership. | RV64 local-store/reload and FPR home publication. | High: easy to mix with local-memory, stack-frame, aggregate, or byval routes. | Defer or reject for Step 3 unless a future fresh scan isolates scalar FPR local-store/reload rows. Do not include `src/bf64-1.c`. |
| 6 | Scalar FPR same-module call/return adjacency | No current scalar-FP bucket evidence in Step 1; branch history only. | No current row assigned to this candidate. | `21a948e9b`, `cb7da5cde`. | Prepared call/result lanes, scalar FPR argument/result homes, call clobber facts, and same-module callee identity. | RV64 scalar call lowering. | High for this runbook because the current bucket evidence does not justify it. | Reject for idea 425 follow-up creation unless new non-F128 rows identify scalar FPR call/return as first owner. |
| 7 | F128/helper ABI/local-memory and `conversion.c`-only work | Excluded from scalar non-F128 impact even when branch history is large. | `conversion.c` branch docs, F128 helper/ABI/local-memory rows, and F128 lane/call work; `src/bf64-1.c` is aggregate/move-bundle owned, not scalar FPR. | F128/helper commits including `f3e08b9ea`, `02c930551`, `0516672fd`, `a8cff27d1`, `d10c8bea3`, `5539dbb20`, `667f2ccef`, `67e7e3f85`; `conversion.c` commits such as `acf9e67ab`, `63f3f888e`, `ac8d94c32`, `022780949`. | F128 lane pairs, helper ABI, aggregate/byval, stack frame, local-memory, or `conversion.c`-specific evidence. | F128/runtime/helper/prepared ABI/local-memory work, not scalar FPR salvage. | Blocking contamination risk. | Quarantine. Do not use as scalar FPR KPI, proof, or follow-up idea input. |

Step 2 conclusion: only the fresh `unsupported_floating_cast` candidate is
clean enough to accept as a first scalar-FPR salvage boundary. The two older
scalar residual rows remain useful supporting evidence but are too composite to
drive the first follow-up by themselves. All F128/helper ABI/local-memory,
aggregate/byval, stack-frame, and `conversion.c`-only material is rejected or
quarantined for this runbook.

## Suggested Next

Delegate Step 3 to define follow-up idea boundaries. Suggested first boundary:
residual scalar F32/F64 cast support for the three fresh
`unsupported_floating_cast` rows, with the two older scalar residual rows used
only as supporting context. Do not create follow-ups for F128/helper ABI/local
memory, aggregate/byval, stack-frame, or `conversion.c`-only work.

## Watchouts

The ranking is intentionally conservative because current mainline scalar-FPR
evidence is small: `3` fresh `unsupported_floating_cast` rows plus `2` older
scalar instruction-fragment residual rows. Step 3 should not promote a broad
scalar-FPR rewrite from `try_gcc_torture` history alone. It should explicitly
reject testcase-shaped proof, expectation rewrites, direct cherry-picks,
`conversion.c`-only evidence, and any F128/helper ABI/local-memory or
aggregate/byval/stack-frame contamination.

## Proof

Docs/classification-only step. No build or CTest was required.

- `git diff --check -- todo.md`
