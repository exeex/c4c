# Current Packet

Status: Active
Source Idea Path: ideas/open/832_hir_aggregate_owner_function_parameter_crash_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: State the aggregate-owner parameter-lowering invariant

## Just Finished

- Step 1 established the parameter-lowering invariant. An aggregate
  `TypeSpec` copied into `LirFunction.params` must derive its owner only from
  module-valid, coherent structured metadata; a copied AST `record_def` is
  not a LIR ownership carrier and must never be dereferenced before native
  ownership normalization.
- In the named object-helper fixture, synthesized `Box__Box` parameter
  `this` has `tag_text_id=1` and namespace context 0, matching the module's
  `Box` owner, but carries a stale/foreign `record_def` with
  `unqualified_text_id=3` and an unreadable `name`. The crash is the
  `typespec_aggregate_owner_key` dereference at `llvm_helpers.hpp:600`,
  reached from `lir_owned_type_spec` while `populate_lir_function_params`
  copies that parameter.

## Suggested Next

- Step 2 only: at the LIR-owned function parameter/return type construction
  seam in `lir_owned_type_spec`, normalize away non-owned AST `record_def`
  metadata before aggregate owner lookup, then retain only a matching
  module-owned structured key/tag. Missing, foreign, or type-incoherent
  owner facts must fail closed rather than use a null/default or rendered
  fallback. Cover valid `Box` ownership plus malformed missing, foreign, and
  incoherent owner facts near the existing HIR-to-LIR coverage.

## Watchouts

- Do not add a null/default owner fallback, suppress the crash, weaken tests,
  or claim baseline clearance. 833 remains parked until this route returns
  accepted focused proof to 831.
- `typespec_aggregate_owner_key` is shared by layout and non-parameter paths;
  keep the repair at owned function type construction unless Step 2 proves a
  narrower verifier companion is necessary.

## Proof

- Evidence predecessor: `ba7958ee4` records the fresh exact 14/14 subset
  failure and the clean backend-enabled `f0fc85e4f^` HIR crash provenance.
  Step 1 must retain that provenance while narrowing the HIR owner seam.
- Step 1 diagnostic reproduction: a debug `frontend_hir_tests` run crashes
  at `typespec_aggregate_owner_key` line 600. Its backtrace is
  `lir_owned_type_spec` -> `populate_lir_function_params` ->
  `test_hir_to_lir_object_helper_callees_prefer_link_name_ids`; the inspected
  `Box__Box::this` fact is `tag_text_id=1`, `record_def` owner text id 3, and
  unreadable `record_def->name`.
