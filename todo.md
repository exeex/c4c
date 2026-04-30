# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Remove Sema Rendered-String Owner And Consteval Lookup Routes

## Just Finished

Step 3 removed rendered-name authority from Sema consteval function lookup when
TextId metadata is available. `lookup_consteval_function_by_name()` now returns
structured lookup when present, then TextId lookup when present, and keeps the
rendered-name map only as fallback when neither semantic route produces a
result. A focused Sema parser static-assert test covers stale rendered-name
disagreement where the reference has TextId metadata but no structured key.

## Suggested Next

Continue Step 3 with the remaining Sema rendered-name owner/member/static
lookup routes, prioritizing helpers where owner-key, declaration, `TextId`, or
other structured metadata is already available and rendered maps can become
fallback only.

## Watchouts

- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- This packet intentionally kept rendered overload lookup as compatibility
  fallback when the reference has no structured overload vector.
- This packet intentionally kept rendered consteval function lookup as
  compatibility fallback when the reference has neither structured nor TextId
  metadata.
- Parser `eval_const_int` callers that have named constants already use
  `std::unordered_map<TextId, long long>`; parser three-argument layout calls
  do not perform named-constant lookup.
- Route deletion of the rendered-name `eval_const_int` compatibility overload
  through `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a
  narrower HIR metadata idea before treating it as parser/Sema closure work.
- Step 3 must not count diagnostics, display, mangle/final spelling, or
  comment-only classification as semantic lookup removal.

## Proof

Ran the delegated proof command:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*consteval.*|cpp_hir_.*consteval.*|cpp_negative_tests_.*consteval.*)$' --output-on-failure) > test_after.log 2>&1`.
It passed with 37/37 tests green. Proof log: `test_after.log`.
