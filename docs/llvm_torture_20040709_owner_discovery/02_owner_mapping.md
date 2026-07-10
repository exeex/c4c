# Owner Mapping

## Question

Do the two LLVM torture rows map to an existing generated follow-up idea, or do
they require a new direct implementation idea?

## Inputs

- Current failure boundary:
  `docs/llvm_torture_20040709_owner_discovery/01_current_failure_boundary.md`.
- Generated follow-up order:
  `docs/backend_baseline_history_triage/follow_up_order.md`.
- Current classification:
  `docs/backend_baseline_history_triage/failure_classification.md`.
- Current generated source-idea state under `ideas/open/` and
  `ideas/closed/`.

Step 1 proves the current first observable boundary for both rows is runtime:
the clang-built binaries exit `0`, while the generated `c2ll` binaries abort
after the compile pipe succeeds. Step 1 does not prove whether the first repair
owner is frontend semantic lowering, prepared/backend handoff, target lowering,
object emission, runtime support, or another shared producer.

## Generated Follow-Up Comparison

| Generated idea | Current state | Owner evidence | Fit for rows 1941 and 1942 |
| --- | --- | --- | --- |
| `ideas/closed/659_rv64_byval_prepared_call_boundary.md` | Closed; remaining dump/object-runtime rows split to ideas 669 and 670. | Owns byval aggregate, preserved pointer args, formal GPR publication, and frame-slot pointer-arg payload evidence from backend rows 150, 151, 154, 155, 165, 207, 208, 209, and 210. | Not proven. The LLVM rows have no current row-specific byval, formal-publication, or pointer-arg payload evidence. |
| `ideas/closed/660_rv64_pointer_local_lowering_route_runtime.md` | Closed after the focused pointer-local route/runtime subset was already passing. | Owns loop-carried pointer postincrement, Duff pointer update, and i16 local-array select/store backend rows 120, 122, 127, 192, 193, and 195. | Not proven. A runtime abort alone does not tie either LLVM row to pointer-local source, destination, or update-order facts. |
| `ideas/closed/661_rv64_prepared_destination_publication.md` | Closed; unresolved dump-contract questions split to ideas 671 and 672. | Owns prepared destination, parameter-home publication, call-result predicate, and return-chain destination rows 92, 103, 109, and 172. | Not proven. Neither LLVM row currently has a prepared destination or parameter-home diagnostic. |
| `ideas/closed/662_prepared_backend_contract_and_cli_publication.md` | Closed after prepared backend contract and CLI publication rows were resolved. | Owns prepared liveness, frame/stack call contracts, prepared printer, prealloc inline asm, and prepared-BIR CLI rows 297, 299, 300, 301, 314, and 318. | Not proven. The LLVM rows compile and reach generated-binary execution; current evidence does not identify a prepared contract or CLI exposure boundary. |
| `ideas/closed/663_prepared_object_data_static_storage_runtime.md` | Retired as a misclassification and routed rows 183 and 184 to callee-saved/live-value ownership. | Final evidence showed coherent static object data and a stale RV64 object-route live-value mismatch. | Not proven. The LLVM rows have no current static object-data, symbol, relocation, or stale live-value evidence. |
| `ideas/open/664_riscv_object_emission_internal_probe.md` | Open; row 256 object-emission scope remains separate, with unrelated full-suite regression follow-up split to idea 673. | Owns RISC-V object emission infrastructure after a focused object-writer probe. | Not proven. The LLVM Step 1 proof used generated binaries from the compile pipe and does not identify an object-writer, relocation, section, or symbol contract failure. |
| `ideas/closed/665_aarch64_instruction_dispatch_internal.md` | Closed after AArch64 dispatch proof and row 322 expectation-contract boundary review. | Owns AArch64 instruction dispatch and AArch64 prepared-BIR publication rows 284 and 322. | Not a fit. The LLVM rows are not AArch64 dispatch/publication rows, and their current symptom is generated-program runtime abort. |
| `ideas/closed/666_rv64_callee_saved_gpr_runtime.md` | Closed after rows 183, 184, and 219 passed in focused callee-saved/live-value proof. | Owns RV64 callee-saved GPR preservation and object-route live-value consumption across calls. | Not proven. A runtime abort does not prove callee-saved GPR or live-across-call corruption for either LLVM row. |
| `ideas/closed/667_rv64_packed_local_member_offsets.md` | Closed because the focused packed-member target already passed. | Owns packed local member offset lowering for row 236. | Not proven. The LLVM rows have no current packed-layout or member-offset evidence. |
| `ideas/open/668_llvm_torture_20040709_research.md` | Active research idea. | Owns deciding whether rows 1941 and 1942 map to an existing owner or require a direct implementation idea. | Current owner for this documentation only. It does not repair implementation behavior. |

## Row Decisions

| Row | Mapping decision | Reason |
| --- | --- | --- |
| `llvm_gcc_c_torture_src_20040709_2_c` | No existing generated implementation owner is proven. | Current evidence proves only a generated-program runtime abort. Historical notes mention earlier `20040709-2.c` relationships to packed FP128/global-initializer and packed bitfield scalar-binop work, but those are closed historical routes and not the generated Step 3 follow-up queue. The current focused run does not expose a row-specific first owner. |
| `llvm_gcc_c_torture_src_20040709_3_c` | No existing generated implementation owner is proven. | Current evidence proves the same runtime-abort boundary. Historical screening once grouped `20040709-3.c` with F32/F64 scalar FP instruction fragments, but current Step 1 evidence does not prove an FP instruction, RV64 consumer, frontend, or runtime-library first owner. |

The two rows share the same first observable boundary, but that is not enough
to name a shared implementation owner. Assigning both rows to RV64 pointer,
prepared contract, object emission, AArch64, callee-saved, packed-member, or
byval ownership would be an unproven symptom-to-owner jump.

## Recommended Direct Implementation Idea

Create a separate direct implementation idea only after this research package
is indexed and accepted. Minimum durable scope:

- Title: LLVM torture `20040709_2.c` and `20040709_3.c` runtime abort owner
  probe.
- Goal: identify and repair the first concrete owner for the generated-binary
  aborts in both rows.
- Required first packet: collect row-specific generated IR, prepared/BIR
  summaries, target assembly/object evidence when applicable, and runtime abort
  details sufficient to distinguish frontend semantics, prepared/backend
  handoff, target lowering, object/runtime support, and harness behavior.
- Acceptance rule: implement only after row-specific evidence names one first
  owner; otherwise split the two rows if their first owners differ.
- Reject signals: no expectation edits, unsupported markers, allowlist changes,
  harness filtering, filename-specific matching, or assignment to a closed
  generated owner without fresh evidence.

This is a direct implementation follow-up proposal, not a new source idea
created by Step 2. The active plan only requires the mapping answer here.
