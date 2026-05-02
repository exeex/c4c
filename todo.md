# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 resolved the ordinary global-scope C++ overload declaration blocker by
adding parser-to-Sema structured source-language metadata on AST nodes and
using it as Sema authority for C++ overload-set eligibility. Parser nodes now
carry `SourceLanguage::Cxx` when produced from C++/C4 mode, while C-mode nodes
remain `SourceLanguage::C`.

Sema now permits ordinary global C++ overload declarations through that
structured source-language carrier, still rejects same-parameter
different-return declarations, and still rejects C declarations. Explicit
linkage blocks now propagate `linkage_spec` to their parsed children, so
`extern "C"` overload declarations remain C-linkage conflicts instead of
being accepted through the C++ source-language carrier.

The source-language/linkage slice was reviewed in
`review/step4_source_language_overload_review.md` and accepted as on-track
Step 4 progress. The full-suite baseline candidate was also accepted: the
canonical baseline now records `2987/2987` passing tests at commit
`fc35474ce`.

## Suggested Next

Continue Step 4 with the next parser-to-Sema metadata handoff gap selected by
the supervisor. Keep the packet narrow and name the exact rendered-string or
missing-carrier route before delegating implementation.

## Watchouts

- The source idea remains active; Step 4 progress is not source-idea closure.
- `SourceLanguage` is declaration provenance, not linkage. Keep
  `linkage_spec` authoritative for explicit `extern "C"` conflict behavior.
- Manually constructed AST tests default zero-initialized nodes to C source
  language unless they are created through `Parser::make_node`.
- Static-member lookup still depends on parser/Sema owner and member metadata.
  A generated `NK_VAR` for `Owner::member` must carry `qualifier_text_ids`,
  `unqualified_text_id`, and the correct owner namespace context when the
  parser has the owner identity; do not restore rendered owner lookup to cover
  missing carriers.
- The remaining `$expr:` re-lex branch is compatibility-only for value args
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
