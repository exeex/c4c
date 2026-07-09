Status: Active
Source Idea Path: ideas/open/627_pointer_stack_result_call_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate Prepared Producer Authority

# Current Packet

## Just Finished

Completed Step 2 producer/consumer authority lookup for idea 627 pointer
stack-result rows.

AST-backed lookup:

- `build_call_result_plan(...)` is
  `c4c::backend::prepare::(anonymous namespace)::build_call_result_plan` in
  `src/backend/prealloc/call_plans.cpp:1046`.
- `PreparedCallResultPlan` and
  `find_prepared_call_result_late_publication(...)` are in
  `src/backend/prealloc/calls.hpp:704` and `:747`.
- The RV64 same-module consumer is
  `c4c::backend::riscv::codegen::(anonymous namespace)::fragment_for_prepared_call`
  in `src/backend/mir/riscv/codegen/object_emission.cpp:4588`, called by
  `fragment_for_prepared_instruction(...)` at `:11301`.

Producer authority:

- `build_call_result_plan(...)` creates `PreparedCallResultPlan` for named call
  results and already records callsite association through
  `PreparedCallResultPlan::instruction_index`; the owning
  `PreparedCallPlan`/lookup supplies function, block, wrapper kind, and
  direct callee identity.
- Source/result ABI authority comes from the after-call move bundle via
  `find_call_abi_binding(..., PreparedMoveDestinationKind::CallResultAbi, ...)`,
  filling `source_storage_kind`, `source_register_name`,
  `source_contiguous_width`, `source_occupied_register_names`,
  `source_stack_offset_bytes`, `source_register_bank`,
  `source_register_placement`, and `source_target_register_identity`; fallback
  from `call.result_abi` publishes the same register source when no bundle
  binding exists.
- Destination/home authority comes from
  `maybe_named_value_id(...)` plus
  `find_prepared_value_home(...)`, filling
  `destination_storage_kind`, `destination_register_name`,
  `destination_target_register_identity`, `destination_slot_id`,
  `destination_stack_offset_bytes`, and `destination_spill_slot_placement`.
  Regalloc lookup fills `destination_value_id` and destination register
  placement when applicable.

Carrier sufficiency:

- Existing `PreparedCallResultPlan` fields are sufficient for the in-scope
  ordinary pointer stack-result shape: `value_bank`, `source_storage_kind`,
  `source_register_name`, `source_register_bank`, `source_contiguous_width`,
  `destination_storage_kind`, `destination_value_id`, `destination_slot_id`,
  `destination_stack_offset_bytes`, `destination_contiguous_width`,
  `source_register_placement`, and `destination_spill_slot_placement`.
- Existing late-publication facts are also sufficient as an admission helper:
  `find_prepared_call_result_late_publication(...)` marks source-register
  publication available when `destination_value_id`, register source storage,
  source register name, and source bank are present. It carries destination
  value id, destination slot id, and destination stack offset through the fact.
- No minimal prepared fact extension is required before Step 3/Step 4. The
  representative Step 1 rows already expose the needed prepared fields.

RV64 consumer authority:

- `fragment_for_prepared_call(...)` already has a stack-slot result branch that
  requires register source storage, source width 1, destination width 1,
  scalar/GPR-compatible value and source banks, source register name,
  destination value id, destination slot id, destination stack offset, and a
  matching prepared value home from `prepared_value_home_for_id(...)`.
- The branch already validates the prepared stack home by requiring
  `destination_home->kind == StackSlot`,
  `destination_home->slot_id == result.destination_slot_id`, and
  `destination_home->offset_bytes == result.destination_stack_offset_bytes`
  before calling `prepared_stack_slot_home_absolute_offset(...)` and
  `append_rv64_store_register_to_stack_offset(...)`.
- The only in-scope blocker is the explicit guard:
  `if (call.result->type == c4c::backend::bir::TypeKind::Ptr) return std::nullopt;`.
  That guard rejects pointer stack-result rows before the existing stack-home
  validation and store emission can run.

Fail-closed condition for the implementation packet:

- Admit only ordinary direct same-module calls already accepted by
  `fragment_for_prepared_call(...)` with `wrapper_kind == SameModule`,
  non-indirect call plan, matching direct callee, and matching planned/result
  presence.
- For pointer stack-result admission, require `call.result->type == Ptr`,
  `call.return_type == Ptr`, `result.value_bank == Gpr` or `None`,
  `result.source_storage_kind == Register`, `result.source_register_bank == Gpr`
  or `None`, `result.source_register_name` present, source and destination
  contiguous widths equal 1, `late_publication.source_register_publication_available`
  or equivalent source-register fields present, `destination_storage_kind ==
  StackSlot`, `destination_value_id`, `destination_slot_id`, and
  `destination_stack_offset_bytes` present, `rv64_register_number(source_reg)`
  present, `prepared_value_home_for_id(...)` present and matching the same
  stack slot/offset, pointer store width exactly 8 bytes on RV64, and
  `prepared_stack_slot_home_absolute_offset(...)` matching the prepared
  destination offset before storing.
- Reject missing, mismatched, non-stack-home, non-GPR/FPR, grouped-width,
  aggregate, byval/outgoing-stack, indirect/variadic/library/runtime,
  local/global-memory, or inferred-from-ABI-register-only rows through the
  existing unsupported same-module call diagnostic path.

## Suggested Next

Step 3/Step 4 can be a narrow implementation packet in
`src/backend/mir/riscv/codegen/object_emission.cpp` plus focused backend test
coverage. Replace the pointer-only rejection in the stack-slot result branch
with an explicit RV64 pointer stack-result admission path using the existing
prepared carrier fields and the fail-closed predicate above. Add positive
coverage for a prepared pointer result from `a0` to a stack slot, plus negative
coverage for at least one missing/mismatched destination authority field.

## Watchouts

- Do not use source-file names, final assembly shape, ABI register names alone,
  or aggregate shape as pointer result-home authority.
- Do not fold scalar-only, scalar frame-slot argument publication, aggregate
  outgoing-stack/byval transport, FPR, variadic, library, runtime,
  local/global, expectation, unsupported marker, allowlist, timeout, or
  accounting work into this idea.
- `20011113-1.c` has both in-scope pointer stack-result rows and an adjacent
  outgoing byval aggregate stack-copy argument bucket.
- `20030715-1.c` and `pr58209.c` expose frame-slot argument publication gaps
  near the pointer result calls; those are adjacent input-argument buckets.
- `pr20601-1.c` has broad local/global pointer memory traffic; use the
  prepared call result facts above for this idea, not the surrounding global
  stores/loads.
- The existing scalar stack-result object tests already reject pointer
  stack-results as a fail-closed shape; Step 3/4 should update that expectation
  only as part of semantic pointer stack-result admission, not as an expectation
  downgrade.
- Treat testcase-shaped shortcuts as route drift.

## Proof

Command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, 347/347 backend tests. Proof log: `test_after.log`.
