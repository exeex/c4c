Status: Active
Source Idea Path: ideas/open/109_bir_prealloc_legacy_compatibility_residue_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Retained Residue Inventory

# Current Packet

## Just Finished

Step 1 seeded the retained BIR/prealloc compatibility residue inventory from
closed idea notes. The inventory below captures the named residue, closed-note
citation, suspected owner, current disposition from the notes, and the evidence
needed before any follow-up can be accepted as real progress.

### Retained Residue Inventory

| Residue | Closed-note citation | Suspected owner | Current disposition from closed notes | Evidence needed before follow-up |
| --- | --- | --- | --- | --- |
| `LegacySlotNameSliceFamilyCompatibility` | `ideas/closed/106_prealloc_stack_layout_slice_family_fact_contract.md` close note names it as the bootstrap producer for scalarized `family.N` local-slot names. | Likely BIR/prepared slice-family authority for durable family identity; prealloc stack layout may retain temporary bootstrap compatibility until that producer exists. | Retained compatibility path, not placement authority by itself. Stack-layout physical placement stays in prealloc. | Current reachability of the legacy name path; whether ordinary scalarized aggregate lowering still lacks a non-legacy BIR-native `PreparedStackSliceFamily` producer; proof that removing/narrowing the path preserves sliced aggregate address publication and malformed-slice fail-closed behavior. |
| `LegacyFrameAddressNameCompatibility` | `ideas/closed/106_prealloc_stack_layout_slice_family_fact_contract.md` close note names it as the bootstrap producer for local frame-address values. | Likely BIR/prepared frame-address authority for durable value identity; prealloc owns frame-slot address materialization and layout consequences. | Retained compatibility path until explicit BIR/prepared frame-address authority exists. | Current reachability of name-derived frame-address value production; whether rooted pointer binary facts fully cover ordinary frame-address publication; proof that any replacement preserves frame-address publication and fails closed without valid rooted facts. |
| Pointer-carrier publication / dereference boundary residue | `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` generated idea 104 for pointer provenance overlap; `ideas/closed/104_bir_prealloc_pointer_carrier_provenance_contract.md` closes with explicit authority for prepared pointer-value access facts, BIR pointer-symbol IDs, and BIR pointer add/sub immediate relations. | BIR owns target-neutral provenance and symbol identity; prealloc regalloc owns transient carrier metadata and physical homes. | Mostly completed contract; retained only unchanged preservation of already-authorized carriers through no-address local slots and explicit computed-pointer authority from authorized add/sub facts. | Current audit of `pointer_carriers.cpp` reachability for any remaining publication or dereference inference from raw load/store order, slot spelling, byte deltas, or missing `MemoryAddress`; evidence that such routes either fail closed or consume explicit BIR/prepared facts. |
| Memory-base publication / dereference boundary residue | `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` retains prepared frame/global/string/pointer addresses and base-plus-offset availability as prealloc placement/addressing authority, while BIR owns local/global memory facts and `MemoryAddress` annotations. `ideas/closed/107_prealloc_inline_asm_memory_effect_metadata_contract.md` closes structured inline-asm memory/address metadata consumption. | BIR owns memory address/provenance facts; prealloc owns prepared target-facing address plans, stack placement consequences, and conservative placement policy. | Completed or retained by boundary: structured metadata should drive object-specific publication/dereference facts; unstructured side-effect summaries are conservative placement only. | Current reachability for any object-specific memory-base publication or dereference facts minted from raw use shape without `MemoryAddress`, inline-asm operand metadata, or prepared address facts; proof should distinguish conservative home-slot reinforcement from semantic provenance. |
| Deferred store-source dump visibility | `ideas/closed/108_prepared_select_chain_dump_contract_coverage.md` close note says store-source dump visibility remains deferred because that route did not add a bounded prepared-module store-source carried fact. | Likely prealloc prepared-printer/lookup contract surface, with BIR source-producer facts consumed as inputs. | Deferred visibility gap; the note explicitly did not create separate store-source continuation work. | Identify whether a bounded prepared-module store-source carried fact now exists or is needed; cite consumer-facing lookup users; proof should expose store-source source-producer/direct-global facts in dumps without broad unrelated printer expansion. |
| Dynamic alloca / VLA no-action notes | `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` treats dynamic alloca as call-like pointer production until target stack adjustment, lifetime, or extent handling proves a distinct fact is needed. `ideas/closed/101_closure_note_followup_recovery_audit.md` marks the dynamic-allocation note stale/no-action because no concrete target-neutral lifetime, extent, or target stack-adjustment fact was found. `ideas/closed/100_bir_runtime_intrinsic_placeholder_identity_contract.md` retains dynamic-stack raw string checks as documented compatibility exceptions outside that contract. | No current follow-up owner without a concrete missing fact. If reopened, BIR would own target-neutral lifetime/extent identity while prealloc/target code owns stack-adjustment and placement consequences. | No-action/stale compatibility residue, not an implementation task yet. | Concrete current route proving dynamic alloca/VLA needs a distinct target-neutral fact: lifetime, extent, stack adjustment, or structured identity. Also evidence that raw dynamic-stack string compatibility is reached by ordinary supported lowering and cannot remain documented compatibility. |
| `prepared_lookups.cpp` helper authority questions | `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` says same-block and source-producer helpers remain query glue unless later work proves semantic memory provenance creation. `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md` rejects broad `prepared_lookups.cpp` rewrites without identifying consumer-facing API gaps. `ideas/closed/108_prepared_select_chain_dump_contract_coverage.md` closes with `select_chain_lookups.cpp` as shared helper authority and printer formatting only. | Prealloc lookup/query surface; possible BIR owner only if a helper creates or re-derives target-neutral semantic facts instead of indexing prepared facts. | Retained query glue/helper authority unless a specific semantic authority leak is proven. | Per-helper current caller map, the fact each helper returns, and whether that fact is produced elsewhere as BIR/prepared authority; proof must name a consumer-facing API gap or semantic re-derivation, not just helper size or file location. |

## Suggested Next

Execute Step 2 by validating the current reachability of the two named stack
layout compatibility paths before proposing any implementation or follow-up
idea.

## Watchouts

- This is analysis-only; do not edit implementation files.
- Do not treat compatibility naming as automatically wrong without current
  reachability evidence.
- Several residues are already closed as completed contracts or explicit
  no-action notes. Reopening them needs fresh current-code evidence, not only
  the existence of old helper names.
- `prepared_lookups.cpp` is explicitly not a cleanup target without a named
  fact or consumer-facing API gap.

## Proof

Analysis-only packet. Ran `git status --short` after editing and confirmed the
only changed file is `todo.md`. No `test_after.log` was produced because the
delegated proof was status-only.
