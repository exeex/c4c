Status: Active
Source Idea Path: ideas/open/prealloc-call-plan-phase-split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Call Plan Surface

# Current Packet

## Just Finished

Completed `plan.md` Step 1: audited the call-plan target surface without
editing implementation, header, test, plan, or source-idea files.

Major helper and declaration map:
- `call_plans.hpp` exposes only `populate_call_plans(PreparedBirModule&)`.
  It is the population entry point and should stay thin unless extracted
  call-planning subphases need non-local declarations.
- `calls.hpp` is the aggregate public contract. It owns the prepared call data
  families: argument plans, result plans, clobbered registers, preserved
  values, wrapper kind, indirect callee plans, memory-return plans, call-plan
  containers, boundary-move classification, and boundary-effect plans.
- `formal_publications.hpp/.cpp` are adjacent formal-entry publication
  planning, not call-site phase planning. They share value-home/publication
  vocabulary but should not be folded into call-plan extraction.
- `prepared_printer/calls.cpp` is a dump mirror over `PreparedCallPlan` data
  and should follow data-family changes only.

Working-model family map:
- Argument planning: `register_bank_from_arg_abi`,
  `aarch64_byval_register_lane_width`, `find_call_abi_binding`,
  `find_call_register_abi_binding`, the per-argument loop in
  `populate_call_plans`, value-home/source-materialization helpers
  (`maybe_named_value_id`, `find_prepared_value_home`,
  `storage_encoding_from_home`, `resolve_symbol_pointer_name`,
  `display_symbol_pointer_name`, `find_f128_constant_regalloc_value`), and
  target-register placement calls through `regalloc_detail`.
- Direct-call planning: `resolve_direct_callee`, `find_module_function`,
  `find_module_function_by_link_name_id`, `classify_call_wrapper_kind`, and
  `variadic_fpr_arg_register_count`; this writes `wrapper_kind`,
  `direct_callee_name`, `is_indirect`, and variadic FPR count before arguments
  are assembled.
- Indirect-call planning: `build_indirect_callee_plan` maps the callee value
  through names, regalloc, value homes, storage encoding, pointer-base/delta,
  immediate, stack/register, and optional placement data. Its inputs are clear:
  names, target profile, optional regalloc function, optional value locations,
  and the call instruction.
- Result planning: `register_bank_from_result_abi`, the after-call bundle path
  in `populate_call_plans`, `move_storage_kind_from_home`,
  `make_spill_slot_placement`, value-home lookup, regalloc lookup, and
  target result placement calls.
- Preservation planning: `CallPreservationCandidates`,
  `make_call_preservation_candidates`, `find_call_program_point`,
  `build_call_preserved_values`, `is_callee_saved_register_assignment`, and
  `find_saved_callee_save_index`; this derives stack-slot and callee-saved
  routes for values live across a call.
- Clobber derivation: `build_call_clobber_set`,
  `find_register_pool_placement`, `find_register_placement`,
  `assignment_register_placement`, and `as_reserved_scratch_placement`; this
  emits caller-saved spans, widened for register groups observed in regalloc.
- Boundary effects: `find_call_boundary_argument_plan`,
  `call_boundary_binding_matches_move`, `find_call_boundary_abi_binding`,
  `call_boundary_result_plan_matches_move`,
  `classify_prepared_call_boundary_move`, endpoint builders,
  `append_explicit_call_boundary_effects`,
  `append_preservation_call_boundary_effects`, and
  `plan_prepared_call_boundary_effects`.
- Memory-return handling: `build_memory_return_plan` owns sret arg discovery,
  prepared slot-name resolution, aggregate subslot fallback by dotted suffix,
  frame-slot selection, and memory size/alignment recording.

Direct and indirect argument planning notes:
- Direct and indirect call argument placement currently share the same
  per-argument loop in `populate_call_plans`; directness affects wrapper and
  optional callee metadata, not the argument source/destination path itself.
- Destination planning has a clear boundary around before-call ABI bindings,
  register-binding fallback, AArch64 byval lane widening, and stack fallback
  when the widened register span cannot be named.
- Source planning has a clear but broad boundary around named value homes,
  pointer-base computed addresses, symbol addresses, immediates, F128 constant
  regalloc lookup, source value IDs, and register placement.
- The direct extraction candidate is not "direct-call arguments" versus
  "indirect-call arguments"; the durable first split is likely destination ABI
  planning and source materialization for all calls, with indirect callee
  planning kept as its own existing helper.

Preservation, clobber, boundary-effect, and memory-return path notes:
- Preservation already has a reasonably isolated helper family and explicit
  inputs. It depends on prepared module, frame plan, liveness, value-home
  lookup, preservation candidates, and call position.
- Clobber derivation is isolated and target-profile driven. It should remain
  tied to caller-saved target facts and regalloc group width, not a
  call-shape-specific rule.
- Boundary-effect planning is already public through `calls.hpp` because AArch64
  and x86 consumers classify and materialize boundary moves from prepared call
  data. Any split here must preserve the aggregate contract or add a proven
  smaller consumer-facing boundary.
- Memory-return handling is isolated enough for a behavior-preserving helper
  extraction, but it is ABI-sensitive and includes an aggregate-subslot fallback
  by slot-name suffix, so it should not be combined with general argument
  cleanup in the same packet.

Public-contract pressure:
- Evidence against splitting `calls.hpp` now: it is included by prealloc module
  state, lookups, storage/runtime/publication surfaces, prepared printers,
  AArch64 call lowering, x86 lowering, and backend compatibility code. Several
  consumers need multiple families from the same aggregate call plan.
- Evidence for possible later split: boundary classification/effect declarations
  are a more focused service than the raw plan structs, and prepared-printer
  rendering only needs read-only dump fields. No independently consumed smaller
  contract is proven in this audit.
- `call_plans.hpp` has low pressure and should remain the population entry
  header. New declarations should be avoided unless `.cpp`-local extraction
  cannot express a stable helper boundary.

Prepared-printer mirror map:
- Header call line mirrors `PreparedCallPlan`: block/index, wrapper kind,
  variadic FPR count, indirect flag, direct callee, indirect callee detail, and
  memory-return detail.
- `append_indirect_callee_detail` mirrors `PreparedIndirectCalleePlan` fields:
  value, encoding, bank, value ID, register, slot, stack offset, immediate,
  pointer base, and delta.
- `append_memory_return_detail` mirrors `PreparedMemoryReturnPlan`: storage
  slot, encoding, sret arg index, frame slot, stack offset, size, and alignment.
- Argument rows mirror `PreparedCallArgumentPlan`: value bank, source encoding,
  source IDs/literals/symbols/register/slot/stack/base/delta, destination
  placement/register/width/units/bank/stack offset.
- Result rows mirror `PreparedCallResultPlan`: value bank, source/destination
  storage, destination ID, source and destination placements/registers/widths,
  destination spill slot, frame slot, and stack offset.
- Preserve rows mirror `PreparedCallPreservedValue`; clobber rows mirror
  `PreparedClobberedRegister`. No label/helper change is required for this
  audit because no data-family change was made.

Stale wording and mutable-state leakage candidates:
- `prepared_printer/calls.cpp` still has a `Step 5 fence` / `legacy raw symbol`
  comment in `source_symbol_name`; this is stale process wording and should be
  cleaned in a later naming-only packet if it is in scope.
- The per-argument block in `populate_call_plans` has broad mutable
  `arg_plan` state and visually fragile nesting around destination fallback,
  byval widening, and source materialization. Extracting by explicit
  input/output helpers would reduce mutation leakage.
- `find_call_abi_binding` prefers stack-slot bindings over the first matching
  binding. That behavior is intentional-looking but implicit; avoid changing it
  during extraction.

Extraction candidates:
- Step 2 candidate: extract all-call argument destination planning into a
  file-local helper that takes target profile, call, arg index, before-call
  bundle, and current value bank, then returns destination fields.
- Step 2 candidate: extract all-call argument source materialization into a
  file-local helper that takes names/module, optional regalloc/value locations,
  target profile, call argument value, and value type context, then returns
  source fields and IDs.
- Step 2 candidate: keep `build_indirect_callee_plan` as the indirect callee
  subphase; do not merge it into argument extraction.
- Step 3 candidate: preservation and clobber helpers are already isolated;
  prefer light naming/placement cleanup over moving public declarations.
- Step 4 candidate: boundary classification/effect helpers may be separable
  behind a focused `.cpp` region or future file, but only after argument/result
  extraction proves the shape.
- Deferred candidate: memory-return helper could be moved or renamed later, but
  should be separated from argument extraction because it encodes sret storage
  and aggregate-slot fallback behavior.

No-edit proof:
- No implementation files, headers, tests, `plan.md`, or source idea files were
  changed for this packet.

## Suggested Next

Execute Step 2 from `plan.md`: perform a small behavior-preserving extraction
inside `src/backend/prealloc/call_plans.cpp` for argument destination planning
or argument source materialization, keeping helpers file-local and leaving
`calls.hpp` unchanged.

## Watchouts

- Keep this slice behavior-preserving.
- Do not change ABI policy, argument/result placement, clobbering,
  preservation, indirect-callee handling, memory-return behavior, formal
  publication behavior, or prepared dump meaning.
- Keep `calls.hpp` as the aggregate public contract unless usage proves a
  smaller independently consumed boundary.
- Reject line-count-only extraction, target-shaped shortcuts, named-case
  handling, or printer label changes that alter printed meaning.
- Preserve the current stack-binding preference in `find_call_abi_binding`.
- Do not split by direct-vs-indirect argument planning unless a real semantic
  difference appears; the audited argument path is shared.
- Treat the memory-return aggregate-subslot fallback as ABI-sensitive and keep
  it out of the first argument extraction packet.

## Proof

Ran `git diff --check`: passed.
