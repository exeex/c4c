# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 removed the rendered `tpl_struct_origin` reconstruction from the
deferred member-typedef base handoff. `TypeSpec` now carries a structured
`tpl_struct_origin_key`, parser template-instantiation producers fill it when
they already have the primary `QualifiedNameKey`, and
`resolve_deferred_member_base()` consumes that key instead of parsing
`resolved_member.tpl_struct_origin` through `qualified_name_from_text(...)`.

The same base-instantiation handoff now finds the pending base primary from
`tpl_struct_origin_key` as well, while retaining the origin spelling only for
instantiation naming/display. Focused lookup-authority coverage now corrupts
the rendered dependent-base `tpl_struct_origin` before instantiation and still
requires the structured key path to attach the concrete inherited `record_def`
before Sema.

The follow-up tightening updated Sema `TypeSpec` equivalence so template-origin
identity compares `tpl_struct_origin_key` whenever either side carries a valid
key, falling back to `tpl_struct_origin` spelling only when both keys are
absent. Focused coverage now proves equal structured keys ignore stale rendered
origin spelling, different structured keys reject, one-sided keys reject, and
the rendered fallback remains limited to the no-key compatibility case.

## Suggested Next

Continue Step 4 by reviewing the remaining parser/Sema compatibility notes for
`$expr:` carriers and choose the next removable rendered/text authority route
that can be proven without HIR edits.

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
- `tpl_struct_origin` remains a compatibility/display spelling and is still
  passed to template-instantiation helpers for emitted names. Do not use it to
  rediscover template primaries or decide Sema `TypeSpec` equivalence when
  `tpl_struct_origin_key` is present.
- The directly adjacent parser change in
  `src/frontend/parser/impl/expressions.cpp` is deliberately narrow: it only
  avoids splitting a concrete instantiated owner TextId when no
  `tpl_struct_origin` carrier is present. Dependent template-origin routes now
  also have `tpl_struct_origin_key` where parser producers already know the
  primary key.
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
