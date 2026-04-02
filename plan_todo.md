# Plan Todo

Status: Active
Source Idea: ideas/open/header_entry_surface_and_function_naming_refactor.md
Source Plan: plan.md

## Active Item

- [ ] Step 3: Rename parser helpers by behavior category

## Todo

- [x] Step 1: Audit parser and HIR naming/grouping surfaces
- [x] Step 2: Normalize entry/header structure
- [ ] Step 3: Rename parser helpers by behavior category
- [ ] Step 4: Rename HIR lowering helpers by semantic role
- [ ] Step 5: Validate navigability and build health

## Next Slice

- Land the first parser rename batch around probe/consume/preflight helpers:
  `check2`, `consume_template_parameter_type_head`,
  `consume_template_args_followed_by_scope`, and the record-member entry
  helpers, with declarations/definitions/call sites updated together.

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
- Validation:
  baseline `test_before.log` and after-run `test_after.log` both finished with
  the same 3 known failures
  (`positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`,
  `llvm_gcc_c_torture_src_20080502_1_c`); the monotonic regression guard
  passed with zero new failing tests.

## Blockers

- None recorded.
