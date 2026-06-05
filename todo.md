Status: Active
Source Idea Path: ideas/open/109_bir_prealloc_legacy_compatibility_residue_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify Current Reachability And Ownership

# Current Packet

## Just Finished

Step 2 closed the `find_local_frame_address_source()` ambiguity in
`src/backend/prealloc/call_plans.cpp` by classifying each reachable fallback
call site against explicit prepared frame-address materialization authority.

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

### Call-Planning Frame-Address Fallback Classification

AST-backed reachability confirms `find_local_frame_address_source()` is called
only from `select_prepared_call_argument_source()` in
`src/backend/prealloc/call_plans.cpp`. The helper itself does not consume a
prepared materialization fact: it scans `prepared.stack_layout.objects` for
`source_kind == "local_slot"`, matches the source value name to the prepared
object name by exact spelling or legacy `.0` lane spelling through
`local_frame_address_name_matches()`, then returns the associated
`PreparedFrameSlot`. Its direct callees are only name spelling,
`prepared_stack_object_name()`, `local_frame_address_name_matches()`, and
`find_prepared_frame_slot()`. Classification: compatibility lookup, not
durable semantic authority.

Fallback 1 is the `FrameSlotAddress` branch for
`argument.source_encoding == PreparedStorageEncodingKind::FrameSlot`. It first
handles the explicit sret memory-return slot, then asks
`find_latest_frame_slot_materialization(addressing, names, block_label,
call_plan.instruction_index, source_home->value_name, false)`. Only if that
exact-name materialization lookup fails does it call
`find_local_frame_address_source()`, and only for a pointer call argument whose
source home and argument source id agree through
`call_argument_uses_local_aggregate_frame_address()`. The producer side is
covered for ordinary call arguments:
`append_address_materializations()` visits every `bir::CallInst` argument and
routes pointer values through `append_pointer_value_address_materialization()`,
which first calls `append_frame_slot_address_materialization()`.
`append_frame_slot_address_materialization()` emits
`PreparedAddressMaterializationKind::FrameSlot` only when the named pointer is
indexed in `frame_slots_by_frame_address_value_name`, which is built from
`PreparedStackObject::frame_address_value_name` after
`apply_frame_address_publication_hints()`. Classification:
`needs-follow-up-idea` candidate. The fallback is reachable, but supported
ordinary local-frame-address lowering should be able to select the explicit
same-call `FrameSlot` materialization instead; keeping the name scan in
`call_plans.cpp` is temporary compatibility ownership unless a later proof
finds an ordinary producer miss.

Fallback 2 is the register-source
`LocalFrameAddressMaterialization` branch for
`argument.allows_local_aggregate_address_publication`,
`argument.source_encoding == PreparedStorageEncodingKind::Register`, and a
register `source_home`. It first normalizes a same-block pointer add/sub result
through `find_same_block_local_frame_address_derived_source()`, then asks
`find_latest_frame_slot_materialization(..., selected_source_value_name, true)`.
The `allow_lane_name` lookup accepts exact and `.0` lane spellings and applies
the derived byte delta before returning. Only after that explicit lookup fails
does the branch call `find_local_frame_address_source()` and reconstruct the
slot offset from stack-layout object names. Producer coverage exists for both
direct and same-block derived ordinary routes: call arguments are visited at
the call instruction, and binary pointer operands are visited earlier by
`append_address_materializations()` through both
`append_frame_slot_address_materialization()` on the lhs and
`append_pointer_value_address_materialization()` on lhs/rhs. Classification:
`needs-follow-up-idea` candidate. The fallback is reachable compatibility glue,
but explicit prepared materializations already model the supported route and
should own the selection.

Fallback 3 is the computed-address
`LocalFrameAddressMaterialization` branch for
`argument.source_encoding == PreparedStorageEncodingKind::ComputedAddress`,
`source_home->kind == PreparedValueHomeKind::PointerBasePlusOffset`, and a
`pointer_base_value_name`. It first asks
`find_latest_frame_slot_materialization(..., selected_source_value_name, true)`
for the pointer carrier base and applies `source_home->pointer_byte_delta`.
Only after that fails does it call `find_local_frame_address_source()` on the
base value name. Current producer evidence says pointer-base-plus-offset homes
come from `regalloc/value_homes.cpp` only when `pointer_carriers.cpp` records
semantic pointer carrier authority. The ordinary binary add/sub carrier path
requires a named pointer base operand, and
`append_address_materializations()` materializes binary pointer operands
before the later call selection. Classification: `needs-follow-up-idea`
candidate with one bounded proof caveat. The name fallback should be
replaceable by explicit prepared materialization for ordinary immediate-offset
pointer carriers, but the follow-up should prove there is no supported
non-binary pointer-carrier authority that gives a base value name without a
prior or same-instruction `FrameSlot` materialization.

Overall ownership classification for
`LegacyFrameAddressNameCompatibility` in `call_plans.cpp`: the stack-layout
legacy name bridge is still reachable, but the remaining call-planning users
are candidates for replacement by explicit prepared frame-address
materialization authority. Supported ordinary aggregate/local-frame-address
lowering appears to be covered by
`PreparedAddressMaterializationKind::FrameSlot` production; no
fallback-by-fallback evidence found a call-planning path that must continue to
derive local frame-address selection from object names.

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

Run the next Step 2 packet against the pointer-carrier publication /
dereference boundary residue. Classify current `pointer_carriers.cpp` and
related consumer reachability for any remaining publication or dereference
inference from raw load/store order, slot spelling, byte deltas, or missing
`MemoryAddress`; distinguish fail-closed query glue from semantic fact
creation.

## Watchouts

- The `call_plans.cpp` fallbacks should not be removed in this audit plan; the
  current plan is analysis-only. A follow-up idea should own any replacement.
- Do not remove or narrow `legacy_slot_name_slice_family_compatibility()` before
  adding a non-legacy `PreparedStackSliceFamily` producer; current code search
  found no other producer.
- `LegacyFrameAddressNameCompatibility` should not be treated as wholly stale:
  prepared materialization lookup exists, but stack-layout still uses the
  legacy name bridge to seed frame-address publication facts.
- The only caveat left on call-planning fallback replacement is the computed
  pointer-carrier branch: follow-up implementation work should first prove no
  supported non-binary pointer-carrier authority lacks a preceding explicit
  frame-slot materialization for its base.
- Several residues are already closed as completed contracts or explicit
  no-action notes. Reopening them needs fresh current-code evidence, not only
  the existence of old helper names.
- `prepared_lookups.cpp` is explicitly not a cleanup target without a named
  fact or consumer-facing API gap.

## Proof

Analysis-only packet. Used `rg` plus `c4c-clang-tool-ccdb
function-signatures`, `function-callees`, and `function-callers` for
`src/backend/prealloc/call_plans.cpp` and
`src/backend/prealloc/stack_layout/coordinator.cpp`; inspected the narrow
producer ranges in `stack_layout/analysis.cpp`, `regalloc/value_homes.cpp`,
and `regalloc/pointer_carriers.cpp`. Final proof command: `git status
--short`. No `test_after.log` was produced because the delegated proof was
status-only.
