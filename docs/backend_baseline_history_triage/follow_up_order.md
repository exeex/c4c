# Backend Baseline Follow-Up Order

Generated for Step 3 of `plan.md` on 2026-07-10. The order is based on
`docs/backend_baseline_history_triage/failure_classification.md`, which is
grounded in `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`,
2026-07-10 04:20:43, reporting `34/3397` failed.

## Ordering Rules

1. Newest broad baseline evidence wins over older summaries.
2. Producer/publication or shared boundary owners precede consumer/runtime
   singleton owners.
3. Wider failed-row families precede narrow or target-specific rows when
   dependencies are otherwise equal.
4. Existing ideas 647, 655, and 657 remain separate. New follow-ups must not
   reopen stack-destination fan-in authority or the `loop-2e.c` representative
   writeback route unless fresh evidence proves those are again first owners.

## Ordered Follow-Ups

| Order | Idea | Owning layer | Baseline rows | Rationale |
| ---: | --- | --- | ---: | --- |
| 1 | `ideas/open/659_rv64_byval_prepared_call_boundary.md` | RV64 byval/prepared call-boundary publication and consumption | 9 | Broadest assigned backend family; call-boundary producer facts can affect dump, route, runtime, and object-runtime rows. |
| 2 | `ideas/open/660_rv64_pointer_local_lowering_route_runtime.md` | RV64 pointer-local lowering for prepared local-address, pointer-step, and store-source cases | 6 | Broad route/runtime family. It depends on distinguishing current non-657 pointer-local rows from the newer 657 representative pass. |
| 3 | `ideas/open/661_rv64_prepared_destination_publication.md` | RV64 prepared destination and parameter-home publication/consumption | 4 | Shared prepared destination/publication symptoms should be probed before narrow singleton runtime cases. |
| 4 | `ideas/open/662_prepared_backend_contract_and_cli_publication.md` | Prepared backend contract publication and CLI dump exposure | 6 | Internal producer/contract rows can explain multiple prepared-BIR and CLI failures, but must stay separate from RV64 runtime lowering. |
| 5 | `ideas/open/663_prepared_object_data_static_storage_runtime.md` | Prepared object-data/static-storage publication and RV64 object-data consumption | 2 | Assigned object-data family with a narrow storage owner after broader prepared/call-boundary work. |
| 6 | `ideas/open/664_riscv_object_emission_internal_probe.md` | RISC-V object-emission backend infrastructure | 1 | Target object-emission row is blocked pending probe and should not be merged into RV64 runtime or prepared CLI work. |
| 7 | `ideas/open/665_aarch64_instruction_dispatch_internal.md` | AArch64 instruction dispatch and prepared-BIR AArch64 publication | 2 | Target-specific AArch64 owner after generic prepared/CLI contract work is separated. |
| 8 | `ideas/open/666_rv64_callee_saved_gpr_runtime.md` | RV64 callee-saved GPR preservation across calls | 1 | Assigned singleton runtime owner with no evidence that it blocks broader producer families. |
| 9 | `ideas/open/667_rv64_packed_local_member_offsets.md` | RV64 packed local member offset lowering | 1 | Assigned singleton runtime owner; keep separate from object-data/static-storage storage publication. |
| 10 | `ideas/open/668_llvm_torture_20040709_research.md` | LLVM torture integration owner discovery | 2 | Step 2 intentionally deferred these rows. They need research before implementation so they are not silently absorbed into a backend family. |

## Duplicate-Idea Boundaries

- Ideas 647 and 655 own prepared/prealloc stack-destination fan-in authority.
  None of the generated follow-ups select ordered final-state,
  mutual-exclusion, explicit-merge, or fan-in rejection authority as their
  implementation route.
- Idea 657 owns the `loop-2e.c` indirect-store/postincrement representative
  route. The generated pointer-local follow-up starts from the current
  baseline's non-representative pointer-local rows and must preserve the
  2026-07-10 04:18 representative pass as current evidence.
- The generated LLVM torture follow-up is research-only because Step 2 did not
  assign a first implementation owner for rows 1941 and 1942.

## Deferred Or Probe-First Rows

- Blocked pending probe rows remain implementation candidates only after their
  source idea records a focused producer/consumer boundary.
- The LLVM torture rows remain intentionally deferred until
  `ideas/open/668_llvm_torture_20040709_research.md` proves whether they share
  an existing backend owner or require a separate frontend/runtime/harness
  implementation idea.

## Step 3 Completion Check

- Follow-up ordering matches the generated source ideas under `ideas/open/`.
- Every generated source idea names one owning layer, in-scope and
  out-of-scope boundaries, acceptance criteria, and concrete
  `## Reviewer Reject Signals`.
- No implementation, tests, expectations, unsupported markers, allowlists,
  runtime behavior, baseline acceptance files, `plan.md`, or existing source
  ideas 647, 655, 657, and 658 were changed by this handoff.
