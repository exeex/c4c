Status: Active
Source Idea Path: ideas/open/109_bir_prealloc_legacy_compatibility_residue_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify Current Reachability And Ownership

# Current Packet

## Just Finished

Step 2 validated current reachability and provisional ownership for the two
named stack-layout compatibility residues.

### Stack-Layout Compatibility Reachability

`LegacySlotNameSliceFamilyCompatibility` is still reachable through the normal
stack-layout route. `BirPreAlloc::run_stack_layout()` calls
`plan_function_stack_objects()` in
`src/backend/prealloc/stack_layout/coordinator.cpp`; that calls
`stack_layout::collect_function_stack_objects()` and then
`apply_aggregate_address_publication_hints()`. Local slots are converted by
`make_local_slot_object()` in
`src/backend/prealloc/stack_layout/analysis.cpp`, which assigns
`.slice_family = legacy_slot_name_slice_family_compatibility(names,
prepared_name)`. The helper parses dotted prepared slot names such as
`%slice.src.0`, interns the family prefix, records the numeric slice offset,
and sets `PreparedStackSliceFamily::legacy_slot_name_compatibility = true`.
Current-code search found no non-legacy producer for
`PreparedStackSliceFamily`: outside tests, the only assignment to
`PreparedStackObject::slice_family` is this helper. Consumers are real:
`apply_aggregate_address_publication_hints()` marks family members
`aggregate_address_published` when the undotted family pointer is used, frame
slot publication indexes `slice_coverage_by_family`, `find_direct_frame_slot()`
uses that coverage to resolve sliced accesses, and `slot_assignment.cpp`
preserves fixed slice-family layout when needed. Provisional classification:
retained bootstrap producer for prepared slice-family identity, not physical
placement authority. Suspected owner is BIR/prepared slice-family production;
prealloc owns the current compatibility bridge and frame placement consequences.

`LegacyFrameAddressNameCompatibility` is also still reachable. The same
`plan_function_stack_objects()` path calls
`apply_frame_address_publication_hints()` in
`src/backend/prealloc/stack_layout/analysis.cpp`; that scans pointer-typed
named values across binary/select/cast/phi/call/load/store/terminator use
shapes, then for a stack object whose prepared slot name appears as a pointer
value it sets `frame_address_value_name` and
`legacy_frame_address_name_compatibility = true`. The producer feeds
`build_frame_slot_publication_facts()` in
`src/backend/prealloc/stack_layout/coordinator.cpp`, which indexes
`frame_slots_by_frame_address_value_name`; `append_frame_slot_address_materialization()`
then publishes `PreparedAddressMaterializationKind::FrameSlot` for matching
pointer values, and `append_address_materializations()` reaches that helper
from binary operands, pointer call arguments, and store-local values.
Downstream consumers now prefer structured materializations where available:
`prepared_lookups.cpp` has value-name and value-id frame-address lookup helpers
that consume indexed `PreparedAddressMaterialization` records and fail closed
on missing, future, non-addressable, or conflicting authority. However,
`call_plans.cpp` still has name-derived fallback ownership:
`find_local_frame_address_source()` matches a source value name to a local-slot
object by exact name or `.0` compatibility, and
`select_prepared_call_argument_source()` uses that fallback for
`FrameSlotAddress` and `LocalFrameAddressMaterialization` selection when a
latest prepared frame-slot materialization is unavailable. Provisional
classification: mixed state. Structured prepared frame-address materialization
authority exists and is consumed, but name-derived local frame-address
selection remains reachable as a compatibility fallback. Suspected owner is
BIR/prepared frame-address value identity plus prepared materialization facts;
prealloc owns the temporary bridge and target-facing slot/address selection.

Exact ambiguity left for later Step 2 packets: current reachability is proven,
but this packet did not prove whether every ordinary local aggregate
frame-address publication now gets an indexed
`PreparedAddressMaterialization` before `call_plans.cpp` selection. The next
packet should classify each remaining `find_local_frame_address_source()`
fallback as either still required by supported lowering or removable after a
bounded prepared-materialization producer check.

## Inventory Context

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

Run the next Step 2 packet against `call_plans.cpp` and addressing
materialization production: prove whether each reachable
`find_local_frame_address_source()` fallback is still required by a supported
ordinary aggregate-address route or can be replaced by explicit prepared
frame-address materialization authority.

## Watchouts

- This remains analysis-only until the fallback-by-fallback ownership question
  is closed.
- Do not remove or narrow `legacy_slot_name_slice_family_compatibility()` before
  adding a non-legacy `PreparedStackSliceFamily` producer; current code search
  found no other producer.
- `LegacyFrameAddressNameCompatibility` should not be treated as wholly stale:
  prepared materialization lookup exists, but call-argument selection still has
  reachable name-derived fallback paths.
- Several residues are already closed as completed contracts or explicit
  no-action notes. Reopening them needs fresh current-code evidence, not only
  the existence of old helper names.
- `prepared_lookups.cpp` is explicitly not a cleanup target without a named
  fact or consumer-facing API gap.

## Proof

Analysis-only packet. Used `rg` plus `c4c-clang-tool-ccdb function-signatures`
for `src/backend/prealloc/stack_layout/analysis.cpp`,
`src/backend/prealloc/stack_layout/coordinator.cpp`,
`src/backend/prealloc/prepared_lookups.cpp`, and
`src/backend/prealloc/call_plans.cpp`. Ran `git status --short` after editing;
the only changed file is `todo.md`. No `test_after.log` was produced because
the delegated proof was status-only.
