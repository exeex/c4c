# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Remove Sema Rendered-String Owner And Consteval Lookup Routes

## Just Finished

Step 3 removed rendered-name authority from the Sema consteval value and
function lookup helpers when structured or `TextId` metadata is available.
`ConstEvalEnv::lookup(const Node*)` now returns structured, then `TextId`, then
rendered fallback values, and consteval function lookup uses the same
structured/`TextId` preference before falling back to `consteval_fns` by name.
Focused stale-rendered tests cover both value lookup and nested consteval
function lookup disagreement.

## Suggested Next

Continue Step 3 with the remaining Sema rendered-name owner/member/static
lookup routes outside the consteval helpers, starting from owner/member/static
call sites that already have declaration, owner-key, `TextId`, or structured
metadata.

## Watchouts

- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- This packet intentionally kept rendered consteval lookup as compatibility
  fallback when neither structured nor `TextId` metadata produces a result.
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
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*consteval.*|cpp_hir_.*consteval.*|cpp_hir_template_deferred_nttp.*|cpp_hir_template_alias_deferred_nttp_static_member)$' --output-on-failure) > test_after.log 2>&1`.
It passed with 38/38 tests green. Proof log: `test_after.log`.
