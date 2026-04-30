# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Remove Sema Rendered-String Owner And Consteval Lookup Routes

## Just Finished

Step 3 removed rendered-name authority from Sema ref-overload and C++ overload
lookup helpers when a call reference carries a structured function key.
`lookup_ref_overloads_by_name()` and `lookup_cpp_overloads_by_name()` now parity
check rendered vs structured vectors, return the structured vector when present,
and keep rendered maps only as compatibility fallback. A focused Sema parser
test covers stale rendered-name disagreement for both ref-overload and C++
overload-set calls.

## Suggested Next

Continue Step 3 with the remaining Sema rendered-name owner/member/static lookup
routes, prioritizing helpers where owner-key, declaration, `TextId`, or other
structured metadata is already available and rendered maps can become fallback
only.

## Watchouts

- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- This packet intentionally kept rendered overload lookup as compatibility
  fallback when the reference has no structured overload vector.
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
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_hir_lookup_tests|frontend_parser_tests|cpp_positive_sema_.*(overload|function|method|static|member|call).*|cpp_negative_tests_.*(overload|function|method|static|member|call).*)$' --output-on-failure) > test_after.log 2>&1`.
It passed with 404/404 tests green. Proof log: `test_after.log`.
