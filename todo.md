# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 made the combined `$expr:` carrier packet acceptance-ready by repairing
the parser-to-Sema member-typedef metadata handoff gap that blocked
`frontend_parser_tests`. Record-body `using` and `typedef` member writers now
publish non-template member typedefs into the structured direct
record/member `QualifiedNameKey` table, without recreating rendered
`Owner::Member` typedef storage.

The using-alias writer also seeds deferred member `TypeSpec` metadata from the
structured alias-template member-typedef carrier when parsing the RHS did not
preserve the member `TextId`, keeping alias-template member typedef metadata
structured across the same Step 4 handoff family. The existing `$expr:`
carrier changes remain intact.

## Suggested Next

Supervisor can review and commit this coherent Step 4 slice: the `$expr:`
token carrier work plus the direct record/member typedef writer repair are now
covered by the delegated proof subset.

## Watchouts

- The source idea remains active; Step 4 progress is not source-idea closure.
- Ordinary global-scope C++ overload declarations are still a separate blocker
  because the current AST handoff does not let Sema distinguish them from C
  global redeclarations without weakening C behavior.
- Static-member lookup still depends on parser/Sema owner and member metadata.
  A generated `NK_VAR` for `Owner::member` must carry `qualifier_text_ids`,
  `unqualified_text_id`, and the correct owner namespace context when the
  parser has the owner identity; do not restore rendered owner lookup to cover
  missing carriers.
- The remaining `$expr:` re-lex branch is now compatibility-only for value args
  with no `ParsedTemplateArg::expr`, no value-arg `TypeSpec::array_size_expr`,
  no `ParsedTemplateArg::nttp_text_id`, and no `captured_expr_tokens`.
- `captured_expr_tokens` is parser-local structured metadata. Do not promote
  stale `nttp_name`/`$expr:` text back to semantic authority when the token
  carrier exists.
- `tpl_struct_origin` remains compatibility/display spelling. Do not use it to
  rediscover template primaries or decide Sema `TypeSpec` equivalence when
  `tpl_struct_origin_key` or structured arg carriers exist.
- Simple type-only pending template args now use `TemplateArgRef` structure;
  nested template-origin args still stay on the legacy display path to avoid
  recursive canonical type keys.
- `src/frontend/parser/impl/declarations.cpp` was touched to keep the
  alias-template member typedef carrier and `TypeSpec` deferred-member
  metadata aligned; that file was required by the adjacent green
  `frontend_parser_tests` assertion after the direct record/member key repair.
- Do not reintroduce rendered qualified-text parsing, `$expr:` debug-text
  semantic authority, string/string_view semantic lookup parameters, fallback
  spelling, expectation downgrades, or named-test shortcuts.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully. CTest passed `887/887` matched tests,
including `frontend_parser_tests`,
`frontend_parser_lookup_authority_tests`,
`eastl_cpp_external_utility_frontend_basic_cpp`, and the
`cpp_positive_sema_.*` subset. Final proof log path: `test_after.log`.
