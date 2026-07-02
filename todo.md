Status: Active
Source Idea Path: ideas/open/425_scalar_fpr_salvage_from_try_gcc_torture.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Scalar FPR Evidence Sources

# Current Packet

## Just Finished

Step 1 - Rebuild Scalar FPR Evidence Sources rebuilt the scalar FPR evidence
from current RV64 gcc_torture docs/artifacts and the `try_gcc_torture` branch
history.

Current bucket evidence sources:

- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md` records the
  fresh 2026-07-01 whole-scan summary: `1467` total, `314` pass, `1153` fail.
  Scalar FP-specific current bucket signal is `unsupported_floating_cast` with
  `3` rows, kept separate from F128.
- `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.full.txt` contain the
  mutable row evidence for that fresh scan. The scalar FP rows found with
  `rg -n "unsupported_floating_cast|floating_cast|f32|f64|float|double"`
  include `src/cvt-1.c`, `src/920618-1.c`, and `src/pr66233.c`, all failing
  with `unsupported_floating_cast: RV64 object route supports only prepared
  FPR width casts and I32/I64-to-F32/F64 integer-to-floating casts`.
- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md` and
  `build/agent_state/unsupported_instruction_fragment_rows.tsv` preserve the
  older 2026-06-30 instruction-fragment row set: `1467` total, `404` pass,
  `1063` fail, with `190` `unsupported_instruction_fragment` rows.
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md` classifies that
  older row set with a `Scalar F32/F64 conversion/op residual` bucket of `2`
  rows: `src/ieee/930529-1.c` and `src/ieee/pr72824.c`.
- Prepared dumps under `build/agent_state/421_step2_prepared/` confirm the
  residual shapes: `src_ieee_930529-1.c.prepared.txt` contains a `double`
  division plus heavy select/local-byte checks, while
  `src_ieee_pr72824.c.prepared.txt` contains `float` local storage, `fptrunc`,
  `llvm.copysign.f32`, and a `ne float` branch condition.

Commands used:

- `git merge-base main try_gcc_torture` -> `0d90e1b6b`.
- `git rev-list --count main..try_gcc_torture` -> `552` branch-only commits.
- `git log --oneline --reverse 0d90e1b6b..try_gcc_torture -- src/backend/mir/riscv/codegen/object_emission.cpp tests/backend/mir/backend_riscv_object_emission_test.cpp src/backend/prealloc/regalloc/call_return_abi.cpp`
- `git show --stat --oneline --no-renames <commit> --`
- `rg -n "unsupported_floating_cast|floating_cast|f32|f64|float|double|F32|F64|ieee/930529-1|ieee/pr72824" build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv build/agent_state/rv64_gcc_c_torture_backend_failed.full.txt build/agent_state/unsupported_instruction_fragment_rows.tsv build/rv64_gcc_c_torture_backend -g 'case.log'`

Relevant `try_gcc_torture` scalar/non-F128 evidence commits:

- Scalar F32/F64 casts: `9d0f64883` publishes floating-cast producer facts;
  `b0700c2c3` lowers prepared FP-to-integer casts; `88076c4ec` lowers F32
  integer casts; `18c41c9c3` lowers int-to-F32 materialization; `92d77cf2c`
  lowers int-to-F64 materialization; `8bbaf8eb7` lowers FPR width casts;
  `f95ec37b9` admits F64-to-F32 trunc immediates.
- Scalar FPR binary ops: `3b8c6691e` lowers RV64 FPR arithmetic objects;
  `c77ae7934` lowers F32 FPR binaries.
- Floating select/materialization: `c7f003c85` lowers floating select
  materialization; `54b013526` lowers same-block floating select;
  `21d6f507c` and `13f54dcb1` cover local-stored select materialization,
  including predicate-local-stored shapes.
- FPR local-store/reload: `0b6b06466` lowers callee-saved FPR frame slots;
  `017890e27` lowers FPR local-store publication; `076a9e432`,
  `35d73dfdc`, and `56fc23309` lower FPR compare local/route-shaped/branch
  reloads.
- Floating branch guard handling: `f4d46728e` lowers floating compare
  branches; `9162d7057` publishes scalar float compare results;
  `c43ad51d2` covers scalar float compare publication; `a3b9bbd66` lowers the
  branch-owned scalar compare handoff; `500bcaac6` advances the scalar branch
  guard route.
- Scalar FPR call/return adjacency that may inform but should not dominate the
  first shortlist: `21a948e9b` consumes scalar FPR return move bundles and
  `cb7da5cde` lowers same-module scalar FPR calls.

Initial candidate separation:

- Non-F128 candidates to carry forward: current `unsupported_floating_cast`
  rows (`src/cvt-1.c`, `src/920618-1.c`, `src/pr66233.c`) and the older scalar
  F32/F64 residual rows (`src/ieee/930529-1.c`, `src/ieee/pr72824.c`). Candidate
  families are scalar F32/F64 cast gaps, scalar FPR binary/comparison/branch
  guards, floating select/materialization, and FPR local-store/reload, but Step
  2 should rank them against the small current row counts before drafting
  follow-up ideas.
- Quarantine or reject for this scalar salvage route: F128 helper/ABI/local
  memory commits such as `f3e08b9ea`, `02c930551`, `0516672fd`, `a8cff27d1`,
  `d10c8bea3`, `5539dbb20`, `667f2ccef`, and `67e7e3f85`; `conversion.c`-only
  F128/helper evidence from branch docs and commits such as `acf9e67ab`,
  `63f3f888e`, `ac8d94c32`, and `022780949`; aggregate/byval or local-memory
  rows like `src/bf64-1.c` where the current failure is move-bundle/aggregate
  or stack-frame owned rather than scalar FPR lowering.

## Suggested Next

Delegate Step 2 to rank the non-F128 scalar FPR candidates by current bucket
impact and rewrite value. Keep the `unsupported_floating_cast` rows and the two
older scalar residual rows separate from F128/helper/local-memory and
`conversion.c`-only evidence.

## Watchouts

The `try_gcc_torture` branch is evidence only, not a cherry-pick source. Current
mainline bucket evidence is small for scalar FPR (`3` fresh
`unsupported_floating_cast` rows plus `2` older scalar instruction-fragment
residual rows), so Step 2 should be skeptical of broad scalar-FPR rewrites and
should prefer narrow ideas only where current rows and ordinary-C value align.
Do not let F128 helper, F128 ABI, F128 local-memory, aggregate/byval, stack
frame, or `conversion.c`-only proof enter the non-F128 scalar shortlist.

## Proof

Docs/evidence-only step. No build or CTest was required.

- Evidence commands listed above.
- `git diff --check -- todo.md`
