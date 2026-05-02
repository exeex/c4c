# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 deleted the retained non-template rendered-owner static-member
acceptance. Sema no longer accepts `Owner::member` by slicing rendered
`Node::name`, rediscovering the owner through `structured_record_key_for_tag`,
or treating a rendered complete struct/union name as enough authority. Static
member lookup now resolves through the reference's structured owner
`qualifier_text_ids` plus `unqualified_text_id`, including inherited static
members through recorded base keys, or fails.

The packet also repaired the adjacent parser handoff for
`Template<qualified-type>::member`: when `parse_base_type()` produces a
concrete instantiated owner without a template-origin carrier, the generated
`NK_VAR` now carries the instantiated record/tag `TextId` as one owner segment
instead of splitting `::` embedded inside the specialization spelling. Existing
template-origin carriers are preserved for dependent trait/value routes.

The lookup-authority regression now includes a non-template
`RenderedOwner::stale` reference with only member TextId metadata and no
structured owner metadata. That case previously passed through rendered owner
text and now fails.

## Suggested Next

Continue Step 4 by reviewing the remaining parser/Sema compatibility notes for
`$expr:` carriers and deferred member-template origin lookup, and choose the
next removable rendered/text authority route that can be proven without HIR
edits.

## Watchouts

- The source idea remains active; Step 4 progress is not source-idea closure.
- Ordinary global-scope C++ overload declarations are still a separate blocker
  because the current AST handoff does not let Sema distinguish them from C
  global redeclarations without weakening C behavior. Preserve C and
  `extern "C"` rejection until a structured language/linkage carrier exists.
- Static-member lookup now depends on parser/Sema owner and member metadata.
  A generated `NK_VAR` for `Owner::member` must carry `qualifier_text_ids`,
  `unqualified_text_id`, and the correct owner namespace context when the
  parser has the owner identity. Do not restore rendered owner lookup to cover
  missing carriers.
- The directly adjacent parser change in
  `src/frontend/parser/impl/expressions.cpp` is deliberately narrow: it only
  avoids splitting a concrete instantiated owner TextId when no
  `tpl_struct_origin` carrier is present. Dependent template-origin routes are
  still left on their existing carrier because runtime trait lowering depends
  on that contract.
- `resolve_deferred_member_base()` still finds the member template primary from
  `resolved_member.tpl_struct_origin` via `qualified_name_from_text(...)` and
  TextId-based lookup because `TypeSpec` does not yet carry a structured
  template-origin key for that member-typedef route. Treat this as retained
  parser metadata debt/compatibility, not completed removal of rendered or
  text-keyed lookup from the path.
- Retained `$expr:` carriers and template default-expression re-lex paths are
  compatibility fallbacks, not completed structured cleanup. Successful Step 4
  routes should continue to use parser/Sema structural metadata where present.
- Do not reintroduce rendered qualified-text parsing, `$expr:` debug-text
  semantic authority, string/string_view semantic lookup parameters, fallback
  spelling, expectation downgrades, or named-test shortcuts.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build succeeded. CTest passed `886/886` matched tests, including
`frontend_parser_lookup_authority_tests`,
`eastl_cpp_external_utility_frontend_basic_cpp`, and the
`cpp_positive_sema_.*` subset. Final proof log path: `test_after.log`.
