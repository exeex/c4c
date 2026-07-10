# 675 Step 1 Post-676/677 Residual Reconciliation

## Inputs

- Accepted baseline: `test_baseline.log`, currently `11/3397` failures.
- Candidate baseline: `test_baseline.new.log`, currently `5/3397` failures.
- Fresh backend proof: `test_after.log`, from:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication$|^backend_'
```

Additional row-322 dump evidence is preserved in:

- `build/agent_state/675_post_676_677_residual_reconciliation/00204_aarch64_prepared_bir.txt`
- `build/agent_state/675_post_676_677_residual_reconciliation/00204_aarch64_prepared_bir.err`
- `build/agent_state/675_post_676_677_residual_reconciliation/00204_aarch64_prepared_bir.status`

## Accepted Baseline Failures

Current `test_baseline.log` failures by stable test name:

- `backend_dump_riscv64_stack_passed_parameter_home_publication`
- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_byval_aggregate_fixed_call`
- `backend_dump_riscv64_byval_preserved_pointer_args`
- `backend_dump_riscv64_function_pointer_return_chain`
- `backend_riscv_object_emission`
- `backend_aarch64_instruction_dispatch`
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
- `llvm_gcc_c_torture_src_20040709_2_c`
- `llvm_gcc_c_torture_src_20040709_3_c`

## Candidate Baseline Failures

Current `test_baseline.new.log` failures by stable test name:

- `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
- `backend_cli_riscv64_call_arg_local_frame_address_materialization`
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
- `llvm_gcc_c_torture_src_20040709_2_c`
- `llvm_gcc_c_torture_src_20040709_3_c`

`test_baseline.new.log` remains diagnostic evidence only. It was not accepted
or modified.

## Fresh Backend Proof Failures

Fresh backend proof in `test_after.log` reports `1/368` backend failures:

- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`

The proof therefore confirms that the 676 and 677 rows are not current backend
proof failures.

## 676 And 677 Settlement Check

- `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
  is settled by closed idea 676. Its closure notes record RV64 link/run proof
  for the generated object plus conversion of the stale expected-failure row
  into a positive object-route contract.
- `backend_cli_riscv64_call_arg_local_frame_address_materialization` is settled
  by closed idea 677. Its closure notes record the RV64 object-route
  `LocalFrameAddressMaterialization` repair and focused passing proof for the
  CLI, route, dump, and object-emission coverage.

Although current `test_baseline.new.log` still lists both stable names, the
fresh backend proof and closure notes settle them for current execution. They
should not be reopened from numeric row movement or stale candidate-baseline
membership alone.

## Row 322 Evidence

Fresh proof failure:

```text
[BACKEND_DUMP_SNIPPET_MISSING] --dump-prepared-bir
Missing snippet: arg index=12 value_bank=vreg source_encoding=frame_slot
source_value_id=2732 source_slot=#3142 source_stack_offset=8288
source_bank=fpr dest_bank=none dest_stack_offset=64
```

The current AArch64 prepared-BIR dump instead emits:

```text
arg index=12 value_bank=vreg source_encoding=frame_slot source_value_id=2728
source_slot=#3142 source_stack_offset=8288 source_bank=fpr dest_bank=none
dest_stack_offset=64 dest_stack_size=16
```

The same current dump shows the selected source is `%t58.48` with
`freshness_authority=prior_preservation`, `freshness_ref_inst=399`,
`freshness_ref_abi=12`, and `freshness_ref_preservation=stack_slot`.

Closed idea 665 records that row 322 no longer has a remaining AArch64
implementation owner under that idea. Its closure note says `%t58.48` /
`2728` is the valid current-call source identity for `arg index=12`, while
`2732` would require the rejected later-call/later-store lookahead route.
`review/row322_later_lane_review.md` rejects that lookahead as temporal route
drift and testcase-shaped evidence rather than a current-call publication
rule.

Current first-owner classification: not a proven backend implementation owner.
The current evidence points to a prepared-BIR dump-contract / expectation
review for row 322 over existing current-call publication facts, unless a
follow-up can prove that `2732` is a valid current-call authority without the
rejected later-call lookahead.

Exact missing probe if more evidence is required before routing: compare the
current-call aggregate carrier source facts for `arg index=12` against the
existing expected snippet and prove, without later-call/store lookahead, whether
`source_value_id=2728` or `source_value_id=2732` is the authoritative
current-call source identity for `source_slot=#3142` at `dest_stack_offset=64`.

## Persistent Non-Backend Residuals

The current backend proof does not exercise the LLVM torture rows, but both
baseline files still list:

- `llvm_gcc_c_torture_src_20040709_2_c`
- `llvm_gcc_c_torture_src_20040709_3_c`

Closed idea 668 classifies those rows as a separate LLVM torture owner
discovery result and rejects assigning them to RV64, prepared CLI, AArch64, or
object-emission owners without focused evidence.
