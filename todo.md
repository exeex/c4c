Status: Active
Source Idea Path: ideas/open/101_closure_note_followup_recovery_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Closure-Note Follow-Up Claims

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by inventorying the closure-note follow-up claims
from ideas 97, 98, and 99, and by listing the current `ideas/open/*.md`
comparison set before any disposition judgment.

Current `ideas/open/*.md` inventory used for comparison:

| Current open idea | Comparison note |
| --- | --- |
| `ideas/open/101_closure_note_followup_recovery_audit.md` | Active recovery audit only. It does not represent any of the note-listed follow-up work below. The old `101_bir_prealloc_missing_call_abi_fallback_boundary.md` name is not currently open. |

Closure-note follow-up claim inventory:

| Source closure note | Follow-up claim | Claimed purpose | Authority domain | Current open status |
| --- | --- | --- | --- | --- |
| `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md` | `ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md` | Recover the runtime intrinsic placeholder identity contract surfaced by the call ABI boundary audit. | BIR/runtime-helper identity contract. | Not present in current `ideas/open/`. |
| `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md` | `ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md` | Recover the missing call ABI fallback boundary named by the call ABI audit. | BIR call ABI facts vs prealloc fallback/repair authority. | Not present in current `ideas/open/`; current open `101` is this recovery audit with a different basename. |
| `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md` | `ideas/open/102_aapcs64_va_arg_payload_shape_authority.md` | Recover the AAPCS64 `va_arg` payload shape authority follow-up. | AAPCS64 variadic payload shape, BIR aggregate facts, and prealloc target-sensitive planning. | Not present in current `ideas/open/`. |
| `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md` | `ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md` | Recover the synthetic helper call ABI authority follow-up. | Prealloc/runtime-helper synthetic call ABI authority. | Not present in current `ideas/open/`. |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | `ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md` | Recover the pointer-carrier provenance contract follow-up. | BIR memory provenance vs prealloc pointer-carrier preparation. | Not present in current `ideas/open/`. |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | `ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md` | Recover the raw global address identity fallback contract follow-up. | Global identity/address materialization fallback boundary. | Not present in current `ideas/open/`. |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | `ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md` | Recover the stack-layout slice-family fact contract follow-up. | Prealloc stack-layout slice facts vs BIR memory semantics. | Not present in current `ideas/open/`. |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | `ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md` | Recover the inline-asm memory effect metadata contract follow-up. | BIR/prealloc inline-asm memory operand and effect metadata. | Not present in current `ideas/open/`. |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | Conditional future dynamic-allocation idea | The note says a future idea should be created only if target stack adjustment, lifetime, or extent handling needs a distinct target-neutral dynamic-allocation fact. | Dynamic alloca lifetime/extent vs target stack adjustment authority. | No current open idea observed for this conditional claim. |
| `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` | Aggregate-copy authority follow-up material | The note records missing aggregate-copy authority as target-facing lowering/codegen follow-up material, not evidence for BIR owning stack-source placement. | Target-facing lowering/codegen aggregate-copy authority. | No current open idea observed for this note. |
| `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md` | `ideas/open/108_prepared_select_chain_dump_contract_coverage.md` | Recover prepared-printer and contract-test visibility for scalar select-chain materialization, direct-global select-chain dependency facts, and direct source-producer provenance. | Prealloc prepared-printer/lookup contract coverage. | Not present in current `ideas/open/`. |
| `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md` | No-action: compare-join continuation work | The close note explicitly says not to create compare-join continuation work from this audit. | Prealloc control/publication planning. | No current open idea observed for this no-action claim. |
| `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md` | No-action: select-materialization join-transfer lookup coverage | The close note explicitly says not to create select-materialization join-transfer lookup coverage work from this audit. | Prealloc lookup/control publication coverage. | No current open idea observed for this no-action claim. |
| `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md` | No-action: direct x86/RISC-V arch implementation work | The close note explicitly says not to create x86/RISC-V arch implementation work before the shared contract follow-up is handled. | Arch backend consumer authority after shared contract work. | No current open idea observed for this no-action claim. |

## Suggested Next

Proceed to `plan.md` Step 2: classify each missing follow-up claim as still
needed as-is, completed by later work, superseded by better-scoped work, or
stale/no-action.

## Watchouts

- This plan is analysis-only unless recovered follow-up ideas are written under `ideas/open/`.
- Do not edit implementation files.
- Do not blindly recreate note-listed follow-ups without checking later closure work.
- Do not keep a constantly displayed plan-review counter line here.
- Original follow-up number `101` collides with the active recovery audit's
  current number but not its basename or intent.
- Some named follow-ups appear to have later closed counterparts; Step 2 should
  use later closure notes as evidence before recreating anything.
- The conditional dynamic-allocation and aggregate-copy notes from idea 98 are
  follow-up material, but they were not listed as generated open ideas.

## Proof

Analysis-only proof per packet: ran `git status --short` before editing and
did not run build/tests because no implementation files were changed. No
`test_after.log` was produced because the delegated proof was inventory-only.
