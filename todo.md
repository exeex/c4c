Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 3 / Step 4 for the next bounded decl-stmt-owned
decl-ref carrier adjacent to the qualified decl-ref route.

- Added parallel HIR `DeclRef::name_text_id` storage for local-declaration
  constructor/copy-constructor `var_ref` carriers and array/local indexing refs
  built in `hir_stmt_decl.cpp`, so these AST-origin decl-refs are no longer
  string-only on the migrated route.
- Added parallel HIR `DeclRef::name_text_id` storage for constructor and
  destructor callee decl-refs emitted by declaration/dtor lowering, keeping the
  emitted `LinkNameId` path separate while giving the carrier its own TU-scoped
  text identity.
- Extended focused HIR proof with a local `Widget widget;` fixture so the
  declaration-lowering route now proves both local-object decl-refs and
  ctor/dtor callee decl-refs preserve `TextId` identity alongside the existing
  link-name path.

## Suggested Next

Keep Step 3 bounded: finish the remaining manual statement-lowering decl-ref
sites in `hir_stmt.cpp` that still bypass `lower_var_expr(...)` for stable
names such as `this`/parameter-backed refs, or stop and choose the next nearby
HIR declaration/tag base-name carrier if that would widen less than a broader
decl-ref sweep.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
