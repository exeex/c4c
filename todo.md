# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 removed the remaining rendered-owner static-member acceptance for
template-specialized references that carry template-argument metadata. Sema now
keeps the legacy rendered-owner completeness escape hatch only for
non-template-specialized references; `NK_VAR` static-member references with
`has_template_args` or `n_template_args > 0` must resolve through structured
owner/member metadata or fail.

The lookup-authority regression now includes a no-metadata
`RenderedOwner<int>::stale` static-member reference that previously could pass
only through rendered owner completeness. Existing alias NTTP and dependent
member-typedef base routes still pass through structured base `record_def`
metadata despite rendered/debug-text drift.

## Suggested Next

Continue Step 4 by reviewing whether the retained non-template rendered-owner
static-member acceptance can be removed or narrowed without crossing into HIR
or lifecycle source edits.

## Watchouts

- The source idea remains active; Step 4 progress is not source-idea closure.
- Ordinary global-scope C++ overload declarations are still a separate blocker
  because the current AST handoff does not let Sema distinguish them from C
  global redeclarations without weakening C behavior. Preserve C and
  `extern "C"` rejection until a structured language/linkage carrier exists.
- Template-specialized static-member lookup now depends on the parser setting
  `has_template_args` or `n_template_args` on the generated reference. If a
  future specialized reference still reaches rendered-owner acceptance, the
  exact missing carrier is that template-specialization marker on the
  static-member `NK_VAR`.
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
