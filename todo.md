# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Remove Sema Rendered-String Owner And Consteval Lookup Routes

## Just Finished

Step 3 made Sema template type-parameter recognition treat recorded
`TextId` metadata as authoritative. `is_known_template_type_param_name` now
stops after a structured name-to-`TextId` mirror miss, including stale rendered
parameter names whose mirror was poisoned by disagreeing `TextId`s, instead of
re-authorizing the rendered parameter-name set. `is_current_template_type_param_name`
keeps rendered compatibility only for matching parameters without structured
metadata. Focused frontend coverage now corrupts same-spelling template
type-parameter `TextId` metadata and verifies Sema rejects the stale rendered
fallback while preserving no-metadata rendered compatibility.

## Suggested Next

Continue Step 3 with the next remaining Sema rendered-name semantic lookup
route that already has structured or `TextId` metadata; prefer a route where
the compatibility fallback can be bounded to no-metadata references without
touching HIR or backend carriers.

## Watchouts

- Some existing parsed method-body positives still lack structured field
  `TextId` metadata, so instance-field lookup must keep rendered compatibility
  fallback when the structured field metadata chain is absent.
- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- Route deletion of the rendered-name `eval_const_int` compatibility overload
  through `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a
  narrower HIR metadata idea before treating it as parser/Sema closure work.
- Step 3 must not count diagnostics, display, mangle/final spelling, or
  comment-only classification as semantic lookup removal.
- Namespace-qualified rendered bridges and synthetic locals without structured
  metadata remain compatibility candidates; do not collapse those routes until
  their producers carry equivalent structured metadata.
- `TypeSpec` cast targets still do not carry their own `TextId`; the current
  template-parameter route can only use the current function parameter metadata
  and the global recorded mirror.

## Proof

Ran the delegated proof command:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*)$' --output-on-failure) > test_after.log 2>&1`.
It passed with 464/464 tests green. Proof log: `test_after.log`.
