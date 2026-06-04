Status: Active
Source Idea Path: ideas/open/101_closure_note_followup_recovery_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Each Missing Follow-Up

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by classifying every Step 1 closure-note follow-up
claim against the current open set and later closed evidence. No Step 3
recovered idea is recommended from this disposition.

Current `ideas/open/*.md` inventory used for comparison:

| Current open idea | Comparison note |
| --- | --- |
| `ideas/open/101_closure_note_followup_recovery_audit.md` | Active recovery audit only. It does not represent any note-listed follow-up below. The old `101_bir_prealloc_missing_call_abi_fallback_boundary.md` basename is now closed, not open. |

Step 2 disposition table:

| Source closure note | Follow-up claim | Classification | Supporting evidence | Step 3 create new idea? |
| --- | --- | --- | --- | --- |
| `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md` | `ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md` | completed by later work | `ideas/closed/100_bir_runtime_intrinsic_placeholder_identity_contract.md` says the runtime/intrinsic placeholder contract is complete: structured `intrinsic`/`inline_asm` metadata is authoritative, invalid-link `llvm.*` display names are documented compatibility only, and prealloc variadic/aggregate-address consumers use shared placeholder/helper queries. | No |
| `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md` | `ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md` | completed by later work | `ideas/closed/101_bir_prealloc_missing_call_abi_fallback_boundary.md` says ordinary lowered calls/functions now require explicit BIR ABI carriers, with only named direct-BIR/bootstrap compatibility repairs retained. | No |
| `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md` | `ideas/open/102_aapcs64_va_arg_payload_shape_authority.md` | completed by later work | `ideas/closed/102_aapcs64_va_arg_payload_shape_authority.md` says AAPCS64 `va_arg` placeholders now publish scalar/aggregate payload ABI metadata and HFA lane count/size, with only constrained destination-home mapping retained in prealloc. | No |
| `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md` | `ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md` | completed by later work | `ideas/closed/103_prealloc_synthetic_helper_call_abi_authority.md` says synthetic i128/f128 helpers are complete as prepared-only runtime legalization artifacts, with prepared helper records and the f128 comparison bridge named and covered. | No |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | `ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md` | completed by later work | `ideas/closed/104_bir_prealloc_pointer_carrier_provenance_contract.md` says pointer-carrier routes are classified and implemented as retained transient prealloc metadata or explicit semantic authority, with local-slot/order inference no longer minting provenance. | No |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | `ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md` | completed by later work | `ideas/closed/105_prealloc_raw_global_address_identity_fallback_contract.md` says every raw global spelling fallback has explicit status, `LinkNameId` remains preferred, and raw no-ID fallback is limited to legacy/raw module globals without structured IDs. | No |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | `ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md` | completed by later work | `ideas/closed/106_prealloc_stack_layout_slice_family_fact_contract.md` says stack-layout slot/slice-family routes are classified as bounded compatibility producers or placement analysis, with structured prepared slice-family/frame-address coverage and named legacy compatibility paths. | No |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | `ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md` | completed by later work | `ideas/closed/107_prealloc_inline_asm_memory_effect_metadata_contract.md` says inline-asm placement consequences now consume structured memory/address operand metadata when available, and the retained unstructured path is conservative placement policy only. | No |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | Conditional future dynamic-allocation idea | stale/no-action | The 98 close note made this conditional only: create a future idea if target stack adjustment, lifetime, or extent handling proves a distinct target-neutral dynamic-allocation fact is needed. No current open idea or later numbered closure note provides that proof; related dynamic stack-source work in `ideas/closed/41_riscv_prepared_edge_publication_dynamic_stack_source_policy.md` and `ideas/closed/44_shared_prepared_dynamic_stack_source_authority.md` covers edge-publication source authority, not this alloca lifetime/extent condition. | No |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | Aggregate-copy authority follow-up material | superseded by better-scoped work | The 98 close note says this is target-facing lowering/codegen material, not evidence for BIR-owned stack-source placement. `ideas/closed/75_shared_aggregate_transport_plan_probe.md` is the better-scoped aggregate-copy authority route and completed shared aggregate-transport authority for a concrete AArch64 byval lane-sensitive path. | No |
| `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md` | `ideas/open/108_prepared_select_chain_dump_contract_coverage.md` | completed by later work | `ideas/closed/108_prepared_select_chain_dump_contract_coverage.md` says prepared dumps now expose selected select-chain and direct-global facts through helper-backed printer formatting and focused tests, without creating compare-join or join-transfer work. | No |
| `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md` | No-action: compare-join continuation work | stale/no-action | The 99 close note explicitly says not to create compare-join continuation work from that audit, and 108 kept that no-action boundary. | No |
| `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md` | No-action: select-materialization join-transfer lookup coverage | stale/no-action | The 99 close note explicitly says not to create select-materialization join-transfer lookup coverage from that audit, and 108 kept that no-action boundary. | No |
| `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md` | No-action: direct x86/RISC-V arch implementation work | stale/no-action | The 99 close note explicitly says not to create direct x86/RISC-V arch implementation work from that audit before shared contract work; 108 completed the shared dump/contract visibility work but did not itself scope an arch backend implementation idea. Any arch rebuild should be a separate initiative, not recovered from this no-action claim. | No |

## Suggested Next

Proceed to `plan.md` Step 4 finalization or plan-owner close decision. Step 3
has no recovered idea files to create because every numbered follow-up is
already closed and the non-numbered/no-action claims do not justify new
`ideas/open/*.md` files.

## Watchouts

- This plan is analysis-only unless recovered follow-up ideas are written under `ideas/open/`.
- Do not edit implementation files.
- Do not blindly recreate note-listed follow-ups without checking later closure work.
- Do not keep a constantly displayed plan-review counter line here.
- Original follow-up number `101` collides with the active recovery audit's
  current number but not its basename or intent.
- Step 3 may be skipped by the supervisor/plan owner if the runbook allows
  no-op creation when no still-needed items remain; otherwise Step 3 should
  record that no files were created.
- The conditional dynamic-allocation claim should not be promoted without a
  concrete target-neutral lifetime, extent, or target stack-adjustment fact.
- The direct x86/RISC-V note from idea 99 may become relevant to a separate
  rebuild initiative after shared contracts are complete, but this audit does
  not provide a scoped arch implementation idea.

## Proof

Analysis-only proof per packet: ran `git status --short` before editing and
did not run build/tests because no implementation files were changed. No
`test_after.log` was produced because the delegated proof was classification-only.
