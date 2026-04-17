Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 4 for the bounded HIR template-call
source-template route in `hir_ir.hpp`, `hir_expr_call.cpp`, and
`frontend_hir_tests.cpp`.

- Added parallel HIR `TemplateCallInfo::source_template_text_id` storage so
  preserved template-call source names are no longer string-only in call
  metadata.
- Threaded template-call source-template interning through `lower_call_expr()`
  and pack-expansion call lowering using `module.link_name_texts` without
  collapsing `TextId`, `SymbolId`, or `LinkNameId`.
- Extended focused HIR proof so template-call metadata resolves its parallel
  `TextId` through the HIR module text table.

## Suggested Next

Keep Step 2 bounded: choose one nearby HIR metadata carrier that still stores
stable TU text as `std::string`, such as `ConstevalCallInfo::fn_name`, instead
of widening into broader registry/index churn.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- `HirTemplateDef::template_param_text_ids` now needs to stay aligned with
  `template_params` anywhere template metadata gets synthesized or rewritten
  after `materialize_hir_template_defs()`.
- The `template_defs` map is still keyed by string for untouched consumers, so
  keep this slice limited to parallel storage; compile-time consumers still use
  `template_params` string spellings for binding lookup.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
