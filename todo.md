Status: Active
Source Idea Path: ideas/open/109_bir_prealloc_legacy_compatibility_residue_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prepare Audit Closure Evidence

# Current Packet

## Just Finished

Step 4 prepared closure evidence for the BIR/prealloc legacy compatibility
residue audit. The final disposition table below covers every audited residue,
maps actionable items to the follow-up ideas created in Step 3, records why the
remaining residues are safe, stale, or no-action, and confirms this audit slice
changed no implementation files.

### Final Audit Disposition

| Audited residue | Final disposition | Follow-up idea path | Closure reason |
| --- | --- | --- | --- |
| `LegacySlotNameSliceFamilyCompatibility` | `safe-compatibility-glue` / retained bootstrap | None | Still reachable as the only current `PreparedStackSliceFamily` producer for dotted scalarized local-slot names. It feeds stack-layout slice coverage and fixed placement, but does not create independent physical placement authority. Removing or narrowing it belongs only after a non-legacy prepared slice-family producer exists. |
| `LegacyFrameAddressNameCompatibility` stack-layout bridge | `safe-compatibility-glue` / retained bootstrap | None | Still reachable in stack layout to seed frame-address publication facts. Prepared frame-address materialization authority now exists downstream, but stack layout still needs the legacy bridge until BIR/prepared publishes durable frame-address value identity directly. |
| `find_local_frame_address_source()` call-planning fallbacks using `LegacyFrameAddressNameCompatibility` | `needs-follow-up-idea` | `ideas/open/110_call_planning_frame_address_materialization_authority.md` | The three remaining call-planning fallbacks are reachable name-derived compatibility lookups. Ordinary local-frame-address lowering appears covered by explicit `PreparedAddressMaterializationKind::FrameSlot` production and lookup, so replacement should be owned by a focused follow-up that proves the computed pointer-carrier caveat before removing the fallback. |
| Pointer-carrier publication / dereference boundary residue | `safe-compatibility-glue` / retained-by-boundary | None | Current carrier propagation requires explicit prepared/BIR authority from prepared pointer-value access, frame-address materialization, pointer-symbol identity, or immediate pointer add/sub from an authorized base. No path was found that publishes or dereferences a carrier solely from raw load/store order, slot spelling, byte deltas, or absent `MemoryAddress`. |
| Memory-base publication / dereference boundary residue | `safe-compatibility-glue` / retained-by-boundary | None | Object-specific pointer/global/string/inline-asm memory facts require structured `MemoryAddress`, inline-asm operand metadata, structured symbol identity, or an existing prepared memory/address fact. The no-address local-slot route only reinforces conservative prepared frame homes and does not mint non-frame provenance. |
| Deferred store-source dump visibility | `needs-follow-up-idea` | `ideas/open/111_store_source_publication_dump_visibility.md` | Store-source source-producer and direct-global select-chain facts now exist as bounded `PreparedStoreSourcePublicationPlan` fields and are consumed by target-side planning, but `prepare::print()` still has no prepared-module dump surface for them. The gap is contract visibility, not missing semantic authority. |
| Dynamic alloca / VLA no-action notes | `stale-no-action` | None | Live support is already represented through `PreparedDynamicStackPlan`, prepared-printer coverage, prepared frame/register policy, and fail-closed AArch64 lowering. No concrete missing target-neutral lifetime, extent, or target stack-adjustment fact was found. Raw helper spellings remain bounded pseudo-intrinsic identity for producing or matching prepared dynamic-stack operations. |
| `prepared_lookups.cpp` / `select_chain_lookups.cpp` helper authority questions | `safe-compatibility-glue` | None | The audited helpers index, select, validate, or package existing prepared/BIR facts. No helper was found creating durable semantic authority, re-deriving memory provenance, or exposing a consumer-facing API gap beyond the separately recorded store-source dump-visibility follow-up. |

No implementation files changed in this audit packet. The only file edited by
Step 4 is canonical `todo.md`.

### Step 2 Evidence Basis

Step 2 classified the `prepared_lookups.cpp` helper authority questions after
the earlier stack-layout, call-planning fallback, pointer-carrier,
memory-base, store-source dump visibility, and dynamic alloca / VLA
classifications. Current helper evidence shows the queried families return
indexed prepared facts, same-block producer context, select-chain dependency
metadata, or copied edge-publication source facts; no current consumer proved a
new semantic fact re-derivation or consumer-facing API gap in
`prepared_lookups.cpp` / `select_chain_lookups.cpp`.

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

### Pointer-Carrier Publication / Dereference Boundary Classification

Step 2 classified the current pointer-carrier publication / dereference
boundary residue around `src/backend/prealloc/regalloc/pointer_carriers.cpp`,
`src/backend/prealloc/regalloc/value_homes.cpp`,
`src/backend/prealloc/prepared_lookups.cpp`, and representative consumers.

AST-backed inventory shows `build_pointer_carrier_map()` is the only exported
producer in `pointer_carriers.cpp`; `build_prepared_value_homes()` in
`value_homes.cpp` is its direct caller, and
`classify_prepared_value_home()` is the direct consumer that can turn a carrier
into `PreparedValueHomeKind::PointerBasePlusOffset`. The only semantic carrier
authorities accepted by `has_semantic_pointer_carrier_authority()` are
`PreparedPointerValueAccess`, `PreparedFrameAddressMaterialization`,
`BirPointerSymbol`, and `BirPointerImmediateOffset`.

Raw load/store order does not create a new carrier from nothing. The initial
`PreparedPointerValueAccess` seeds come only from prepared memory accesses whose
address has `PreparedAddressBaseKind::PointerValue` and a
`pointer_value_name`. Prepared frame-address seeds come only from explicit
`PreparedAddressMaterializationKind::FrameSlot` records with a result value and
frame slot. BIR pointer-symbol seeds require a named pointer result carrying
`pointer_symbol_link_name_id`. Immediate add/sub carriers require an existing
semantic base carrier and a named pointer binary result. Classification:
explicit prepared/BIR authority is required before a carrier is publishable.

The local-slot loop preserves and propagates already-authorized carriers across
unaddressed local pointer loads/stores, but it does not infer publication or
dereference authority from slot spelling alone. `StoreLocalInst` and
`LoadLocalInst` propagation is skipped for instructions with `address`; a load
can resolve through `slot_pointer_carriers` only when the slot is not a
multi-store CFG-cycle risky slot, and the slot map is populated only from a
stored value that already resolved to a semantic carrier. Classification:
retained physical preservation/query glue for no-address local pointer slots,
not durable semantic provenance creation. Ambiguous cycle slots fail closed by
withholding slot-carrier propagation.

Missing `MemoryAddress` is not used to mint object-specific dereference facts.
Pointer-indirect prepared memory access production in
`stack_layout/coordinator.cpp` requires `address.has_value()` with
`MemoryAddress::BaseKind::PointerValue`; `prepared_address_has_complete_base()`
requires the prepared pointer base name; source-memory publication lookup copies
only fields from an existing `PreparedMemoryAccess` and reports
`MissingPreparedMemoryAccess` or `IncompletePreparedMemoryAccess` when the
prepared fact is absent or incomplete. `value_homes.cpp` also records local
pointer-load results that are used as prepared pointer memory bases, then
prevents those loaded values from being reclassified as
`PointerBasePlusOffset`. Classification: dereference-side facts consume
structured `MemoryAddress` / prepared memory-access authority or fail closed;
missing `MemoryAddress` remains compatibility local-slot storage, not
semantic dereference authority.

Byte deltas are derived only from explicit immediate BIR pointer add/sub
relations after a semantic base carrier is found. Consumers treat deltas as
computed-address materialization detail rather than independent authority:
`publication_plans.cpp` marks `PointerBasePlusOffset` as `ComputedAddress` and
requires both base and delta, `call_plans.cpp` passes the base/delta through
and first prefers explicit frame-slot materialization for local aggregate
addresses, AArch64 memory lowering uses prepared frame-address lookup or a
prepared register/stack home before emitting the address, and RISC-V edge
publication requires a prepared register base. Classification: no current
consumer evidence found a dereference or publication path whose main authority
is raw byte-delta arithmetic without a prepared/BIR base fact.

Overall classification for the pointer-carrier publication / dereference
boundary residue: mostly retained-by-boundary, not a current semantic leak.
`pointer_carriers.cpp` still preserves already-authorized pointer carriers
through no-address local pointer slots, and that route is reachable, but it is
guarded by explicit carrier authority and cycle ambiguity checks. Related
prepared lookup and backend consumers consume explicit BIR/prepared facts or
fail closed on missing/incomplete authority. No current path was found that
publishes or dereferences a pointer carrier solely from raw load/store order,
slot spelling, byte deltas, or absent `MemoryAddress`.

### Memory-Base Publication / Dereference Boundary Classification

Step 2 classified the current memory-base publication / dereference boundary
residue around `src/backend/prealloc/stack_layout/coordinator.cpp`,
`src/backend/prealloc/prepared_lookups.cpp`,
`src/backend/prealloc/inline_asm.cpp`, publication-plan/source-memory lookup
paths, and representative AArch64 consumers.

AST-backed inventory shows `PreparedMemoryAccess` production is concentrated
in the stack-layout coordinator helpers: `build_direct_frame_slot_access()`,
`build_direct_symbol_backed_access()`, and `build_pointer_indirect_access()`.
The pointer-indirect route is fail-closed on missing structure:
`build_pointer_indirect_address()` returns no prepared address unless the BIR
instruction has `MemoryAddress::BaseKind::PointerValue` with a named pointer
base. The direct global/string route requires either a structured
`MemoryAddress` global/string base or an existing structured global/link-name
fallback from the BIR load/store instruction; unresolved symbol identity or
policy returns no prepared access. Classification: prepared memory-access
production consumes explicit `MemoryAddress` or structured symbol/address
facts for object-specific non-frame bases; it does not mint pointer/global
dereference authority from raw use shape.

The direct frame-slot route is the one reachable no-address compatibility
case. `build_direct_frame_slot_access()` accepts no-address local loads/stores
only by resolving the instruction's local slot through
`find_direct_frame_slot()` and existing `FrameSlotPublicationFacts`, including
slice-family coverage when applicable. It then emits a
`PreparedAddressBaseKind::FrameSlot` access for an existing prepared frame
slot. Classification: conservative home-slot / stack-layout reinforcement,
not semantic memory-base provenance. It can identify a physical stack home for
local-slot traffic, but it does not create pointer, global, string, or
inline-asm dereference facts without the corresponding structured authority.

Source-memory publication facts are copied from existing prepared memory
accesses and fail closed when those facts are absent or incomplete.
`apply_source_memory_access_fact()` applies only to
`PreparedEdgePublicationSourceProducerKind::LoadLocal`, requires producer
block/instruction coordinates, calls `find_prepared_memory_access()`, then
sets `MissingPreparedMemoryAccess` if no access exists. After
`copy_source_memory_access_fact()` copies fields from the found
`PreparedMemoryAccess`, the helper marks the fact incomplete unless the access
result matches the publication source value, `prepared_address_has_complete_base()`
accepts the base, and size/alignment are nonzero. AArch64
`prepared_publication_source_memory_access()` revalidates the publication,
load instruction, prepared access identity, and every copied address field
before using source-memory facts. Classification: publication-plan/query glue,
not independent semantic authority; missing or incomplete prepared access
blocks the source-memory route instead of reconstructing a base from raw
publication shape.

Inline-asm memory/address operands are also guarded by structured metadata and
prepared homes. BIR carries `InlineAsmOperandMetadata::memory_address` and
`address`; prealloc `make_prepared_inline_asm_operand()` copies those fields
into `PreparedInlineAsmOperand`, and `validate_inline_asm_carrier()` records
missing facts such as `*_memory_address_authority` or `*_address_authority`
when operand metadata is absent. Both memory and address operands require
`inline_asm_pointer_address_is_prepared_selectable()`: the metadata address
must be `MemoryAddress::BaseKind::PointerValue`, its base value must equal the
operand value, and the value must already have a prepared register home.
AArch64 `make_inline_asm_memory_address_operand()` repeats those checks before
constructing a prepared `MemoryOperand`, and
`lower_inline_asm_instruction()` rejects memory/address operands with missing
metadata. Classification: structured inline-asm operand metadata plus prepared
home authority is required; unstructured inline-asm side effects and raw
constraint/use shape do not publish object-specific dereference facts.

Representative AArch64 consumers preserve the same boundary. Edge-copy source
memory lowering requires an available and field-matching prepared source
memory access when the publication claims source-memory authority; otherwise
it either fails that route or falls back to non-memory value-home/register
publication. Inline-asm printing accepts only prepared base+offset memory
operands. Aggregate move/source-memory consumers check prepared support,
frame-slot identity, prepared byte-offset snapshots, and printable prepared
addresses before using source memory. Classification: consumers consume
prepared memory/address facts or conservative homes and fail closed on missing
authority; no consumer evidence found object-specific dereference/publication
facts derived solely from raw load/store order, inline-asm text/constraints,
or absent `MemoryAddress`.

Overall classification for the memory-base publication / dereference boundary
residue: `safe-compatibility-glue` / retained-by-boundary. The remaining
no-address local-slot route is conservative frame-home reinforcement owned by
prealloc stack layout, while object-specific pointer/global/string/inline-asm
memory bases require `MemoryAddress`, inline-asm operand metadata, structured
symbol identity, or an existing prepared memory/address fact. Publication
plans and backend consumers copy and validate those facts, or report
missing/incomplete authority and fail closed.

### Deferred Store-Source Dump Visibility Classification

Step 2 classified the deferred store-source dump visibility residue from
`ideas/closed/108_prepared_select_chain_dump_contract_coverage.md`. The close
note says scalar select-chain materialization visibility now prints
`source_producer`, `source_producer_block`, and `source_producer_inst`, and
call-argument direct-global dependency lines now print
`direct_global_select_chain`, `direct_global_source`,
`direct_global_root_is_select`, and `direct_global_root_inst`; it also states
store-source dump visibility remained deferred because that route did not add a
bounded prepared-module store-source carried fact.

Current code has a bounded store-source carried fact at the publication-plan
layer, not at the prepared-module/printer layer. `publication_plans.hpp`
defines `PreparedStoreSourcePublicationPlan` fields for
`source_producer_kind`, source-producer block/instruction coordinates, producer
instruction pointers, `direct_global_select_chain_source`,
`direct_global_select_chain_root_is_select`, and
`direct_global_select_chain_root_instruction_index`.
`plan_prepared_store_source_publication()` in
`src/backend/prealloc/publication_plans.cpp` copies those fields from
`PreparedStoreSourcePublicationInputs` and from the supplied
`PreparedEdgePublicationSourceProducer`. This fact is bounded to an individual
store-source publication plan, but it is currently a transient planning result,
not a section persisted inside `PreparedBirModule` and printed by
`prepare::print()`.

The producer/query route exists and is store-source-specific. AST-backed
signature inventory shows `select_chain_lookups.cpp` exports
`make_prepared_edge_publication_source_producer_lookups()`,
`find_indexed_prepared_edge_publication_source_producer()`,
`find_prepared_direct_global_select_chain_dependency()`, and
`find_prepared_store_source_direct_global_select_chain_dependency()`. The
store-source helper is a narrow wrapper over the direct-global select-chain
dependency query, so it consumes the same BIR source-producer map instead of
inventing new semantic authority. Current-code search shows the production
consumer in AArch64 `plan_store_local_source_publication()`:
it obtains the source producer with `prepared_store_source_producer()`, builds
or reuses `edge_publication_source_producers`, calls
`find_prepared_store_source_direct_global_select_chain_dependency()`, and passes
both source-producer and direct-global fields into
`plan_prepared_store_source_publication()`.

Consumer-facing users are real. In AArch64 memory lowering,
`lower_store_local_value_publication()` requires a store-source plan and then
branches on complete source-producer facts for cast, select materialization,
scalar floating binary, global-symbol load-local, recovered/byval load-local,
and direct-global select-chain cases. It checks source-producer kind,
block label, and instruction index before using producer-specific records, and
uses `direct_global_select_chain_root_instruction_index` to avoid duplicate
materialization around select-chain roots. Other consumer evidence includes
`lower_stack_homed_pointer_value_load_publication()`,
`lower_pointer_base_plus_offset_store_local_publication()`,
`lower_store_global_value_publication_from_plan()`, and the x86 reuse test
adapter in `tests/backend/mir/backend_x86_store_source_publication_plan_reuse_test.cpp`,
all consuming `PreparedStoreSourcePublicationPlan` availability and fields.

Printer visibility is still absent. `prepare::print()` calls
`append_call_plans()` and `append_select_chain_materializations()` but has no
store-source publication-plan printer section. Exact string search in
`src/backend/prealloc/prepared_printer/` and
`tests/backend/bir/backend_prepared_printer_test.cpp` finds only call-argument
direct-global labels and scalar select-chain source-producer rows, with no
`store_source` / `PreparedStoreSourcePublicationPlan` dump surface. The focused
MIR tests prove the facts exist in planning:
`backend_store_source_publication_plan_test.cpp` records cast source-producer
publication, store-source direct-global select-chain dependency fields, and
missing-input fail-closed behavior. The prepared-printer tests do not expose
those store-source facts.

Provisional classification: `needs-prepared-dump-visibility`. No current
evidence suggests a new BIR semantic fact is needed before visibility work:
BIR/prepared source-producer and direct-global select-chain queries already
feed the store-source plan, and consumers use the carried fields. The remaining
gap is contract visibility: add a bounded prepared-printer/module dump surface
for store-source publication-plan facts, scoped to source-producer and
direct-global select-chain fields, without broad unrelated printer expansion.

### Dynamic Alloca / VLA No-Action Classification

Step 2 classified the dynamic alloca / VLA no-action residue from
`ideas/closed/101_closure_note_followup_recovery_audit.md` and the source idea
inventory. The closed note treated the old dynamic-allocation concern as
stale/no-action because it did not identify a concrete target-neutral lifetime,
extent, or target stack-adjustment fact. Current code now has explicit
dynamic-stack operation handling, but that evidence supports the same
`stale-no-action` disposition for this residue rather than a new broad
follow-up idea.

The BIR producer path is concrete and narrow. Dynamic LIR alloca is preserved
only when `LirToBirOptions::preserve_dynamic_alloca` is enabled; then
`BirFunctionLowerer::lower_local_memory_alloca_inst()` lowers counted alloca
to a pointer-returning helper call named `llvm.dynamic_alloca.<type>` with the
count as the first argument and records the resulting pointer address locally.
Stack state operations lower through
`BirFunctionLowerer::lower_call_family_inst()`, which converts
`LirStackSaveOp` and `LirStackRestoreOp` to `llvm.stacksave` and
`llvm.stackrestore` BIR calls. CLI route evidence in
`tests/backend/bir/CMakeLists.txt` still exercises the ordinary VLA path:
`vla_goto_stackrestore.c` must dump BIR containing `llvm.stacksave`,
`llvm.dynamic_alloca.i32`, and `llvm.stackrestore`, and the prepared dump must
show matching dynamic-stack operations for `accumulate_vla`.

Prealloc publishes bounded prepared authority for those helper calls instead
of leaving target consumers to rediscover lifetime or stack-state behavior from
raw strings alone. AST-backed inventory shows `populate_dynamic_stack_plan()`
is the exported producer in `src/backend/prealloc/dynamic_stack.cpp`; its
direct callees include `is_dynamic_alloca_call()`,
`dynamic_alloca_type_text()`, value-name lookup, block-label normalization, and
`find_prepared_frame_plan()`. It scans BIR call instructions for
`llvm.stacksave`, `llvm.stackrestore`, and `llvm.dynamic_alloca.*`, then
records `PreparedDynamicStackOp` entries with function, block label,
instruction index, kind, result value, operand value, and dynamic-allocation
type text. `PreparedDynamicStackPlanFunction` also records
`requires_stack_save_restore` and `uses_frame_pointer_for_fixed_slots`, the
latter copied from the prepared frame plan.

Frame and register-allocation consumers treat dynamic stack as a prepared
placement policy input, not as a missing BIR semantic fact. `frame_plan.cpp`
sets `PreparedFramePlanFunction::has_dynamic_stack` when a stack-save,
stack-restore, or dynamic-alloca helper is present and requires fixed frame
slots to use a frame-pointer base when dynamic stack and fixed slots coexist.
`regalloc.cpp` uses `function_has_dynamic_stack_operation()` to keep active
register assignments live across non-call boundaries unless values start at a
call point, preventing dynamic stack movement from invalidating fixed-slot or
home assumptions. This is target-facing placement and lifetime-preservation
policy derived from existing BIR helper operations plus prepared frame/layout
facts, not evidence of a missing target-neutral lifetime or extent fact.

Prepared visibility already exists for the bounded fact. `prepare::print()`
calls `append_dynamic_stack_plan()`, and the printer emits the
`--- prepared-dynamic-stack-plan ---` section with function, stack-save/restore
requirement, fixed-slot frame-pointer policy, operation block, instruction
index, kind, result, operand, allocation type, and optional element
size/alignment fields. Function summaries also report `has_dynamic_stack` and
dynamic-stack operation counts. Current prepared-printer tests for the VLA
route require the dynamic-stack plan section and operation lines, so this is
not a dump-visibility gap like the deferred store-source publication residue.

AArch64 target lowering consumes prepared dynamic-stack authority and fails
closed when it is absent. AST-backed caller evidence shows
`lower_dynamic_stack_helper_call()` is reached from both
`dispatch_prepared_block()` and `lower_call_instruction()`. Its direct callees
include `dynamic_stack_helper_kind()`, `find_dynamic_stack_op()`,
`lower_dynamic_stack_save()`, `lower_dynamic_alloca()`, and
`lower_dynamic_stack_restore()`. It first recognizes the helper spelling, then
requires a matching `PreparedDynamicStackOp` at the current instruction index;
without that prepared authority it emits a missing-value-authority diagnostic
and a deferred-unsupported machine node before unresolved LLVM helper calls can
reach assembly.

The concrete target stack adjustment is currently target-owned and bounded.
`lower_dynamic_alloca()` requires a printable nonzero element size, prepared
operand and result homes, and stable frame-pointer-backed stack homes when
fixed slots are involved. It computes the byte count from the prepared count
home and allocation type, subtracts from `sp`, aligns the new stack pointer,
updates `sp`, and publishes the allocation result through its prepared home.
`lower_dynamic_stack_save()` and `lower_dynamic_stack_restore()` similarly use
prepared homes to move `sp` to or from a register or stable frame slot. Tests in
`backend_aarch64_instruction_dispatch_test.cpp` verify both fail-closed missing
prepared authority and supported target-owned output without unresolved
`llvm.stacksave`, `llvm.dynamic_alloca`, or `llvm.stackrestore` assembly
references.

The remaining raw dynamic-stack string checks are reached by ordinary lowering,
but current code has documented and bounded compatibility around those
pseudo-intrinsic spellings rather than an exposed semantic gap. BIR still uses
`llvm.dynamic_alloca.<type>`, `llvm.stacksave`, and `llvm.stackrestore` as the
operation identity, and prealloc/AArch64 recognize those spellings to build or
match prepared dynamic-stack operations. However, target consumers that lower
or reject the calls require `PreparedDynamicStackPlan` authority, prepared value
homes, and prepared frame policy; x86 handoff docs also state that VLA and
dynamic `alloca` handling must come from `dynamic_stack_plan` and must not
rediscover lifetime or fixed-slot anchoring from raw call names.

Overall classification for the dynamic alloca / VLA no-action residue:
`stale-no-action`. Current code does expose live dynamic-stack handling, but it
does not expose a concrete missing target-neutral lifetime, extent, or target
stack-adjustment fact that needs a new follow-up idea. Dynamic-stack operations
are already represented by a prepared plan with printer coverage and
fail-closed AArch64 consumption; target stack adjustment remains target-owned,
and the raw helper spellings are bounded compatibility identity for producing
or matching the prepared plan.

### `prepared_lookups.cpp` Helper Authority Classification

Step 2 classified the helper authority questions around
`src/backend/prealloc/prepared_lookups.cpp`,
`src/backend/prealloc/select_chain_lookups.cpp`, and representative AArch64
consumers. AST-backed symbol inventory shows `prepared_lookups.cpp` exports
the broad function lookup/index surface, while `select_chain_lookups.cpp`
exports source-producer and select-chain helpers declared through
`publication_plans.hpp`. Exact call-site search found all queried families are
reachable, but as consumers of prepared/BIR facts rather than producers of new
semantic authority.

The basic indexed fact helpers are retained query glue. `find_frame_slot_by_id()`
and `find_stack_object_by_id()` return existing `PreparedFrameSlot` /
`PreparedStackObject` records by prepared IDs. `make_prepared_value_home_lookups()`
indexes existing `PreparedValueHome` records by value ID and name, while the
value-home lookup consumers use those tables to avoid repeated scans.
`make_prepared_call_plan_lookups()`, `make_prepared_move_bundle_lookups()`,
`make_prepared_return_chain_lookups()`, and their `find_indexed_*` helpers
return existing `PreparedCallPlan`, `PreparedCallArgumentPlan`,
`PreparedMoveBundle`, `PreparedMoveResolution`, after-call lane binding,
return-chain terminal, or return-chain next-operand facts. Some helpers retain
fallback scans when an index is not supplied, but they still return records
from prepared call/value-location data. Classification:
`safe-compatibility-glue`; no helper in this family creates BIR/prealloc
semantic facts.

The prepared memory-access lookup family is also retained query glue.
`make_prepared_memory_access_lookups()` indexes existing
`PreparedMemoryAccess` records by block/instruction position, result value
name, and result value ID. `find_indexed_prepared_memory_access()`,
`find_unique_indexed_prepared_memory_access_by_result_value_name()`, and
`find_unique_indexed_prepared_memory_access_by_result_value_id()` return those
records or fail closed on missing/ambiguous indexes. Current call sites include
`publication_plans.cpp` for source-memory publication planning and AArch64
`alu.cpp` for result-value-ID memory access lookup. The helpers do not derive
memory bases from instruction shape; the underlying `PreparedMemoryAccess`
producer requires prepared addressing, stack-layout, or symbol facts.
Classification: `safe-compatibility-glue`.

The prepared frame-address lookup helpers consume explicit prepared address
materializations, with bounded validation, and are retained query glue.
`find_indexed_prepared_frame_address_offset_for_value()` selects the latest
same-block `PreparedAddressMaterializationKind::FrameSlot` for the requested
value name at or before the query instruction, resolves the prepared frame
slot, rejects negative offsets, rejects non-addressable stack objects, and
fails closed on conflicting same-instruction materializations.
`find_indexed_prepared_frame_address_offset_for_value_id()` performs the same
query by value ID, using `PreparedValueHomeLookups` only to map name-only
materialization records to value IDs when needed. Exact callers are AArch64
`memory_store_retargeting.cpp`, `memory.cpp`, and `globals.cpp`.
Classification: `safe-compatibility-glue`; these helpers select and validate
explicit prepared frame-address materialization facts rather than recreating
frame-address authority from names.

The same-block scalar/source-producer helpers are retained producer-context
query glue. `make_prepared_edge_publication_source_producer_lookups()` scans
the prepared BIR function and indexes named `LoadLocal`, `LoadGlobal`, `Cast`,
`Binary`, and `SelectInst` results by value name with block label,
instruction index, and instruction pointer. `find_prepared_same_block_scalar_producer()`
and `find_prepared_select_chain_source_producer()` require a same-block
producer before the consumer instruction, verify the producer pointer still
matches the indexed BIR instruction, and verify the result value/type/name.
Representative consumers include AArch64 dispatch producers, FP value
materialization, comparison, memory, calls, and prealloc publication planning.
Classification: `safe-compatibility-glue`; these helpers recover bounded
producer context for already-named BIR results, not provenance or storage
authority.

The select-chain helpers in `select_chain_lookups.cpp` are retained query glue
with one already-recorded visibility consequence. `find_prepared_direct_global_select_chain_dependency()`
and `find_prepared_store_source_direct_global_select_chain_dependency()` walk
same-block source-producer chains and report whether the chain contains a
direct global load, plus root select-ness and root instruction index.
`find_prepared_scalar_select_chain_materialization()` returns root value name,
root instruction index, root-is-select, and the direct-global dependency.
Current callers include prealloc `publication_plans.cpp` and `call_plans.cpp`,
prepared-printer select-chain output, and AArch64 `alu.cpp`, `memory.cpp`, and
`calls.cpp`. Classification: `safe-compatibility-glue` for authority, with the
separate store-source result above still classified
`needs-prepared-dump-visibility` because store-source plan fields are not yet
printed.

The same-block load-local stored-value helper is retained guarded query glue,
not memory-provenance creation. `find_prepared_same_block_load_local_stored_value_source()`
first finds a same-block `LoadLocal` producer for the queried value, then
requires an existing prepared load memory access whose result matches the load
and whose address resolves to a prepared frame slot. It walks preceding
same-block `StoreLocalInst` operations and only returns a stored value when the
store has a prepared memory access, the stored value matches the prepared
access, the load/store prepared frame-slot ranges overlap exactly, and
conflicting or partial overlaps fail closed. The exact external caller is
AArch64 `calls.cpp`. Classification: `safe-compatibility-glue`; the helper
uses existing prepared memory-access and stack-layout facts to answer a
same-block copy-source question.

The edge-publication source-fact helpers are retained copied-fact/query glue.
`make_prepared_edge_publication_lookups()` builds `PreparedEdgePublication`
records from prepared control-flow transfer, value-home, source-producer,
memory-access, move, and aggregate-copy authority; `find_unique_indexed_prepared_edge_publication()`
returns a unique indexed publication or fails on missing/ambiguous entries.
`prepare_edge_copy_source_facts()` copies source value, value-home, producer,
memory-access, destination, move, and edge labels from an existing available
publication and validates missing source value/home/producer or missing and
incomplete source memory access. `prepare_block_entry_parallel_copy_edge_source_facts()`
adds block-entry parallel-copy move checks before returning the same copied
source facts. Exact callers include AArch64 `dispatch_edge_copies.cpp`, and
`prepare_current_block_join_parallel_copy_source_facts()` consumes the
block-entry variant. Classification: `safe-compatibility-glue`; these helpers
package and validate publication facts for consumers instead of inventing
edge-copy authority.

The prior-preserved call helper family is retained dominance/index glue.
`make_prepared_call_plan_lookups()` indexes prior preserved values from
prepared call plans and computes block dominance from prepared control-flow
facts. `find_latest_indexed_prior_preserved_value()`,
`find_dominating_indexed_prior_preserved_value()`,
`find_unique_indexed_prior_preserved_value_source()`,
`find_latest_indexed_prior_stack_preserved_value_before_instruction()`, and
`first_indexed_stack_preserved_values_for_call()` return existing
`PreparedCallPreservedValue` records filtered by instruction order, dominance,
or uniqueness. Current consumers are call/result preservation and target
publication paths that need an already-planned preserved value. Classification:
`safe-compatibility-glue`.

Overall classification for the `prepared_lookups.cpp` helper authority
questions: `safe-compatibility-glue`, with the separate store-source dump
visibility classification preserved as `needs-prepared-dump-visibility`.
Current evidence did not find a helper that owns durable target-neutral
semantic facts that BIR/prepared producers should publish elsewhere. The
helpers are reachable and consumer-facing, but their authority is indexing,
same-block context recovery, uniqueness/ambiguity filtering, and copied
prepared-fact packaging over existing prepared/BIR records. No additional
Step 2 reachability gap remains in the inventory.

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

Supervisor should call `to_subagent: c4c-plan-owner` for the closure decision:
decide whether this exhausted audit runbook should close, deactivate, or be
replaced now that Step 4 has consolidated final evidence and created the two
follow-up ideas for actionable residue.

## Watchouts

- The `call_plans.cpp` fallbacks should not be removed in this audit plan; the
  current plan is analysis-only. Follow-up idea
  `ideas/open/110_call_planning_frame_address_materialization_authority.md`
  should own any replacement.
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
- `pointer_carriers.cpp` still has a reachable no-address local slot
  preservation loop. Treat that as retained compatibility/query glue unless a
  later packet finds it creating carriers without one of the four explicit
  authorities.
- `PreparedValueHomeKind::PointerBasePlusOffset` is a computed-address carrier,
  not a storage home that all consumers support. Several consumers deliberately
  fail closed or require a prepared base register/frame-address lookup.
- Direct no-address local-slot `PreparedMemoryAccess` production is retained
  frame-home reinforcement. Do not treat it as a semantic memory-base leak
  unless a later packet finds a consumer using it to infer pointer/global/string
  provenance outside prepared stack-layout authority.
- Inline-asm side-effect summaries and raw constraints are not enough for
  object-specific memory facts; current code requires structured operand
  metadata and prepared register homes for memory/address operands.
- Store-source source-producer and direct-global select-chain facts currently
  exist as transient `PreparedStoreSourcePublicationPlan` fields consumed by
  target-side planning. Follow-up idea
  `ideas/open/111_store_source_publication_dump_visibility.md` should treat the
  live gap as dump/contract visibility, not as missing semantic authority,
  unless a later implementation packet proves the printer needs a new
  persistent prepared-module fact container.
- Dynamic alloca/VLA support is live but already routed through
  `PreparedDynamicStackPlan`; do not turn the old no-action note into broad
  cleanup unless a later packet identifies a concrete target-neutral lifetime,
  extent, or stack-adjustment fact that is missing from the prepared contract.
- Raw dynamic-stack helper spellings are still the BIR pseudo-intrinsic identity
  used to produce and match prepared dynamic-stack operations. Treat that as
  bounded compatibility unless a target consumer lowers those helpers without
  prepared dynamic-stack operation authority.
- Several residues are already closed as completed contracts or explicit
  no-action notes. Reopening them needs fresh current-code evidence, not only
  the existence of old helper names.
- `prepared_lookups.cpp` is explicitly not a cleanup target without a named
  fact or consumer-facing API gap.
- The current `prepared_lookups.cpp` / `select_chain_lookups.cpp`
  classification found query glue, not semantic authority leakage. Do not turn
  helper size, cross-file reachability, or same-block producer context into a
  cleanup idea unless a future packet names a concrete fact re-derived outside
  prepared/BIR producers.
- Store-source select-chain helper authority is safe query glue, but
  store-source plan facts still lack prepared-printer/module dump visibility.
  Keep that as a visibility follow-up, not a source-producer authority rewrite.

## Proof

Analysis-only Step 4 packet. Consolidated closure evidence in canonical
`todo.md` and changed no implementation files. Final proof commands for this
packet: `git diff --check` and `git status --short`. No `test_after.log` was
produced because the delegated proof was analysis/status-only.
