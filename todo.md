Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 3 / Step 4 for the remaining bounded
manual statement-lowering decl-ref sites in `hir_stmt.cpp`.

- Added parallel HIR `DeclRef::name_text_id` storage for statement-built
  parameter-backed refs such as `this` and the defaulted-method `other`
  carrier, so these manual `hir_stmt.cpp` decl-refs are no longer string-only
  on the migrated route.
- Added parallel HIR `DeclRef::name_text_id` storage for delegating-constructor
  and member-constructor callee decl-refs emitted by statement lowering,
  keeping the emitted `LinkNameId` path separate while giving the carrier its
  own TU-scoped text identity.
- Extended focused HIR proof with a constructor/delegation/defaulted-assignment
  fixture so the statement-lowering route now proves `this`, parameter-backed
  decl-refs, and ctor callee decl-refs preserve `TextId` identity through the
  HIR-owned text table.

## Suggested Next

Keep Step 2 bounded: choose the next nearby HIR declaration/tag base-name
carrier that still stores stable TU text as `std::string`, rather than widening
into a broader decl-ref sweep outside the already-covered parser/expr/stmt
routes.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- Manual statement-lowering decl-ref builders can bypass `lower_var_expr(...)`,
  so future `DeclRef` additions in `hir_stmt.cpp` or adjacent helpers need
  their own explicit `name_text_id` interning when they carry stable TU text.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
