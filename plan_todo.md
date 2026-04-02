# Plan Todo

Status: Active
Source Idea: ideas/open/header_entry_surface_and_function_naming_refactor.md
Source Plan: plan.md

## Active Item

- [ ] Step 3: Rename parser probe/consume and record-preflight helpers by behavior category

## Todo

- [x] Step 1: Audit parser and HIR naming/grouping surfaces
- [x] Step 2: Normalize entry/header structure
- [ ] Step 3: Rename parser helpers by behavior category
- [ ] Step 4: Rename HIR lowering helpers by semantic role
- [ ] Step 5: Validate navigability and build health

## Next Slice

- Continue Step 3 with the next parser rename batch around the remaining
  record-definition lifecycle helpers and any still-misnamed declarator-side
  probes/consumers, keeping each rename family in one declaration/definition /
  call-site slice.

## Notes

- Activation selected the first eligible open idea in deterministic filename
  order because no numeric priority prefixes were present.
- Keep this plan isolated from parser-debug / STL observability work, which
  remains in its own open idea.
- Update this file whenever the active step changes or a rename batch lands.
- Audit snapshot:
  `parser.hpp` mixes public entry points, parser state, namespace plumbing,
  template utilities, declarator parsing, and record parsing in one long
  declaration run; the highest-signal parser rename candidates are `check2`
  (next-token probe), `consume_template_parameter_type_head`
  (probe-plus-consume helper), `consume_template_args_followed_by_scope`
  (token-advancing scope matcher), `prepare_record_member_entry`
  (record-member preflight), and the record setup trio
  `parse_record_prebody_setup` / `initialize_record_definition` /
  `complete_record_definition`.
- Audit snapshot:
  `hir_lowerer_internal.hpp` mixes pipeline orchestration, deferred-template
  resolution, callable lowering, initializer lowering, and template
  instantiation helpers in a mostly chronological block; the clearest HIR
  rename candidates are `resolve_pending_tpl_struct_if_needed` /
  `resolve_pending_tpl_struct`, `resolve_struct_member_typedef_hir`,
  `populate_hir_template_defs`, `run_consteval_template_seed_fixpoint`, and
  `record_seed`.
- Completed in this slice:
  added file-role comments and implementation maps to `parser.hpp`,
  `hir.hpp`, and `hir_lowerer_internal.hpp`; regrouped `hir_lowering.hpp`
  around shared entry points/result carriers/utilities; and replaced stale
  helper-divider comments in `hir_lowerer_internal.hpp` with responsibility
  sections.
- Current iteration target:
  rename the parser next-token probe / template-head consumers /
  record-member setup helpers so their names state whether they only peek,
  consume on success, or initialize record-definition state.
- Completed in this iteration:
  renamed `check2` -> `peek_next_is`,
  `consume_template_parameter_type_head` ->
  `consume_template_parameter_type_start`,
  `consume_template_args_followed_by_scope` ->
  `consume_template_args_before_scope`, and
  `prepare_record_member_entry` -> `begin_record_member_parse`; updated the
  declarations, definitions, and parser call sites together; and added a
  focused parse-only regression covering template-template parameter heads plus
  qualified template-id member usage.
- Validation:
  targeted parser tests covering the renamed helpers passed, including the new
  `cpp_parse_template_template_scope_member_dump` case.
- Validation:
  full-suite `test_before.log` and `test_after.log` both finished with the same
  3 known failures
  (`positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`,
  `llvm_gcc_c_torture_src_20080502_1_c`); the monotonic regression guard
  passed with zero new failing tests.

## Blockers

- None recorded.
