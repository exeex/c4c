Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 4 for the bounded HIR struct/tag carrier
route in `hir_types.cpp` and `hir_templates_struct_instantiation.cpp`.

- Added parallel HIR `HirStructDef::tag_text_id` and `base_tag_text_ids`
  storage so non-template and instantiated-template struct definitions are no
  longer string-only for their stable TU-scoped tag identity.
- Threaded HIR-owned text interning through both struct-definition
  construction sites, keeping the sidecar ids aligned with `tag` and
  `base_tags` without touching `SymbolId` or `LinkNameId` policy.
- Extended focused HIR proof with a derived-struct plus instantiated-template
  fixture so both direct struct tags and inherited base tags resolve through
  the HIR text table on the migrated route.

## Suggested Next

Keep Step 2 bounded: choose one nearby declaration-side HIR carrier that still
stores stable TU text as `std::string`, such as `Param.name` or
`HirTemplateDef.name`, instead of widening into broader registry/index churn.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- `HirStructDef::base_tag_text_ids` is a sidecar vector and must stay
  index-aligned with `base_tags` anywhere new base-tag producers are added.
- Template-struct instantiation has its own base-tag construction helper, so
  future struct/tag `TextId` work needs to cover both ordinary and instantiated
  definition paths together.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
