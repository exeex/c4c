# Backend Baseline Failure Classification

Generated for Step 2 of `plan.md` on 2026-07-10. Timestamps are filesystem
modification times in UTC. The row source is
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`,
2026-07-10 04:20:43, which reports `34/3397` failed. The row inventory is
cross-checked against
`docs/backend_baseline_history_triage/evidence_timeline.md`, 2026-07-10
04:35:27.

## Classification Rules

- Each current failed row from the newest baseline appears exactly once below.
- "First owning layer" means the earliest currently justified repair layer, not
  the final implementation target.
- `assigned` means the current evidence is enough to hand the row to a family.
- `blocked pending probe` means the row belongs to a family, but needs a
  focused probe before implementation.
- `intentionally deferred` means Step 3 should decide whether to generate a
  follow-up now or leave it behind a higher-priority producer/consumer family.

## Evidence Used

| Timestamp | Path | Use |
| --- | --- | --- |
| 2026-07-10 04:35:27 | `docs/backend_baseline_history_triage/evidence_timeline.md` | Current row inventory and reverse-chronological authority. |
| 2026-07-10 04:30:09 | `ideas/open/658_backend_baseline_history_umbrella_triage.md` | Required family split and umbrella scope. |
| 2026-07-10 04:20:43 | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log` | Authoritative failed-row source. |
| 2026-07-10 04:18:53 | `build/agent_state/657_step3_representative_pointer_value_store/summary.md` | Newest 657 representative state; `loop-2e.c` representative passes. |
| 2026-07-10 04:03:37 | `build/agent_state/657_step2_destination_writeback_facts/summary.md` | Pointer-value store ownership model for repaired 657 representative. |
| 2026-07-10 03:52:16 | `build/agent_state/656_step4_representative_proof/summary.md` | Current stack-destination fan-in fail-closed diagnostic shape. |
| 2026-07-09 20:19:54 | `build/agent_state/647_step2_family_revision/summary.md` | Residual prepared/prealloc destination fan-in authority context. |

## Family A: RV64 Prepared Destination And Publication

First owning layer: RV64/prepared destination publication and destination-home
consumption. These rows are grouped because their labels name prepared
destination, parameter-home publication, call-result predicate, or return-chain
destination behavior rather than local pointer lowering or byval aggregate
copying.

| Test ID | Row | Status | Evidence path/timestamp | Notes |
| ---: | --- | --- | --- | --- |
| 92 | `backend_dump_riscv64_stack_passed_parameter_home_publication` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Parameter-home publication row. Needs focused producer/consumer boundary before implementation. |
| 103 | `backend_dump_riscv64_scalar_compare_frame_slot_destination` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Prepared destination/frame-slot compare row. |
| 109 | `backend_dump_riscv64_prepared_fused_compare_call_result_predicate` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Prepared control-flow predicate row; keep separate from pointer-local runtime rows. |
| 172 | `backend_dump_riscv64_function_pointer_return_chain` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Return-chain/function-pointer destination row; no newer row-specific probe found. |

## Family B: RV64 Pointer-Local Lowering

First owning layer: RV64 lowering for prepared local-address, pointer-step, and
store-source cases. The current representative `loop-2e.c` pointer-value store
owner is repaired according to the 04:18 evidence, so these are not assigned to
657 directly. They remain a pointer-local route/runtime family until focused
probes identify the exact producer or consumer boundary.

| Test ID | Row | Status | Evidence path/timestamp | Notes |
| ---: | --- | --- | --- | --- |
| 120 | `backend_codegen_route_riscv64_loop_carried_pointer_postincrement` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `build/agent_state/657_step3_representative_pointer_value_store/summary.md`, 2026-07-10 04:18:53 | Route-side pointer-local/postincrement row; 657 representative pass prevents treating `loop-2e.c` as the current broad owner. |
| 122 | `backend_codegen_route_riscv64_duff_fallthrough_pointer_update_producers` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Route-side pointer update producer row. |
| 127 | `backend_codegen_route_riscv64_i16_local_array_select_store` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Route-side i16 local-array select/store row. |
| 192 | `backend_rv64_runtime_riscv64_loop_carried_pointer_postincrement` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `build/agent_state/657_step3_representative_pointer_value_store/summary.md`, 2026-07-10 04:18:53 | Runtime counterpart to row 120; needs non-657 representative probe. |
| 193 | `backend_rv64_runtime_riscv64_duff_fallthrough_pointer_update_producers` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Runtime counterpart to row 122. |
| 195 | `backend_rv64_runtime_riscv64_i16_local_array_select_store` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Runtime counterpart to row 127. |

## Family C: RV64 Byval And Prepared Call Boundary

First owning layer: prepared call-boundary/byval aggregate source and
destination publication, followed by RV64 consumption. These rows share byval,
aggregate, formal-publication, or prepared-call-boundary labels.

| Test ID | Row | Status | Evidence path/timestamp | Notes |
| ---: | --- | --- | --- | --- |
| 150 | `backend_dump_riscv64_byval_aggregate_fixed_call` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Dump-side fixed byval aggregate call row. |
| 151 | `backend_codegen_route_riscv64_byval_aggregate_fixed_call` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Route-side counterpart to row 150. |
| 154 | `backend_dump_riscv64_byval_preserved_pointer_args` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Dump-side prepared call-boundary pointer arg row. |
| 155 | `backend_codegen_route_riscv64_byval_preserved_pointer_args` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Route-side counterpart to row 154. |
| 165 | `backend_codegen_route_riscv64_byval_formal_gpr_publication` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Formal-publication byval row. |
| 207 | `backend_rv64_runtime_riscv64_byval_aggregate_fixed_call` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Runtime counterpart to fixed byval aggregate call. |
| 208 | `backend_rv64_runtime_riscv64_byval_preserved_pointer_args` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Runtime counterpart to prepared-call-boundary pointer args. |
| 209 | `backend_rv64_runtime_riscv64_byval_formal_gpr_publication` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Runtime counterpart to formal GPR publication. |
| 210 | `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Object-runtime prepared call-boundary/pointer payload row. |

## Family D: Prepared Object Data And Static Storage

First owning layer: prepared object data/static local storage publication or
RV64 object data consumption.

| Test ID | Row | Status | Evidence path/timestamp | Notes |
| ---: | --- | --- | --- | --- |
| 183 | `backend_obj_runtime_rv64_prepared_object_data_static_local_storage` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Static local object-data row. |
| 184 | `backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Initialized static local object-data row. |

## Family E: RV64 Callee-Saved GPR Runtime

First owning layer: RV64 prepared GPR/callee-saved preservation across calls.

| Test ID | Row | Status | Evidence path/timestamp | Notes |
| ---: | --- | --- | --- | --- |
| 219 | `backend_obj_runtime_rv64_callee_saved_gpr_live_across_call` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Single callee-saved GPR object-runtime row. |

## Family F: RV64 Packed Local Member Runtime

First owning layer: RV64 packed local member offset lowering.

| Test ID | Row | Status | Evidence path/timestamp | Notes |
| ---: | --- | --- | --- | --- |
| 236 | `backend_rv64_runtime_packed_local_member_offsets` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Single packed local member runtime row. |

## Family G: Internal Prepared BIR, CLI, Object Route, And AArch64

First owning layer: internal backend infrastructure rather than RV64 runtime
lowering. Keep these separated from RV64 object-runtime families so Step 3
does not generate a mixed RV64/AArch64/CLI idea.

| Test ID | Row | Status | Evidence path/timestamp | Notes |
| ---: | --- | --- | --- | --- |
| 256 | `backend_riscv_object_emission` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Internal RISC-V object-emission row; not enough evidence to merge with any RV64 runtime family. |
| 284 | `backend_aarch64_instruction_dispatch` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | AArch64 instruction-dispatch row; separate target owner. |
| 297 | `backend_prepare_liveness` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `build/agent_state/647_step2_family_revision/summary.md`, 2026-07-09 20:19:54 | Prepared/prealloc liveness internal row; may be adjacent to destination-authority work but needs a focused probe. |
| 299 | `backend_prepare_frame_stack_call_contract` | blocked pending probe | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `build/agent_state/647_step2_family_revision/summary.md`, 2026-07-09 20:19:54 | Prepared frame/stack call contract row; do not merge into byval runtime without proof. |
| 300 | `backend_prepared_printer` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Prepared printer internal row. |
| 301 | `backend_prealloc_inline_asm` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Prealloc inline-asm internal row. |
| 314 | `backend_cli_dump_prepared_bir_exposes_contract_sections` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | CLI prepared-BIR dump contract row. |
| 318 | `backend_cli_dump_prepared_bir_local_arg_call_contract` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | CLI prepared-BIR local-arg call contract row. |
| 322 | `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication` | assigned | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | CLI prepared-BIR/AArch64 publication row. |

## Family H: LLVM Torture Rows

First owning layer: unclassified LLVM torture integration until a probe decides
whether these share an existing backend owner or need a separate frontend,
runtime, or harness follow-up.

| Test ID | Row | Status | Evidence path/timestamp | Notes |
| ---: | --- | --- | --- | --- |
| 1941 | `llvm_gcc_c_torture_src_20040709_2_c` | intentionally deferred | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Defer until Step 3 decides whether these belong behind a higher-priority backend owner. |
| 1942 | `llvm_gcc_c_torture_src_20040709_3_c` | intentionally deferred | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`, 2026-07-10 04:20:43; `ideas/open/658_backend_baseline_history_umbrella_triage.md`, 2026-07-10 04:30:09 | Same row family as 1941. |

## Coverage Check

Current baseline failed rows accounted for exactly once:

- Family A: 4 rows.
- Family B: 6 rows.
- Family C: 9 rows.
- Family D: 2 rows.
- Family E: 1 row.
- Family F: 1 row.
- Family G: 9 rows.
- Family H: 2 rows.

Total: 34 rows.

## Step 2 Completion Check

- Every current failed row from
  `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log` appears exactly
  once above.
- The classification keeps RV64 prepared destination/publication,
  pointer-local lowering, byval/prepared call-boundary, prepared object
  data/static storage, callee-saved GPR, packed local member, internal
  prepared BIR/CLI/AArch64, and LLVM torture rows in separate first-owner
  families.
- Rows without row-specific probes are marked `blocked pending probe` or
  `intentionally deferred`; no implementation, test, expectation, unsupported
  marker, allowlist, runtime behavior, baseline acceptance, `plan.md`, or
  source-idea change is implied by this classification.
