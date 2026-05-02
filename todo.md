# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired and tightened one parser-to-Sema forwarded NTTP metadata
handoff gap. Parser template argument nodes now carry a parallel
`template_arg_nttp_text_ids` carrier for simple forwarded NTTP arguments, and
Sema consteval call-env binding consults that `TextId` metadata before the
legacy rendered NTTP name map. The follow-up removed the structured lookup's
accidental dependency on `outer_env.nttp_bindings`, so `nttp_bindings_by_text`
alone can satisfy forwarded NTTP binding. Focused lookup-authority coverage
now proves both stale rendered spelling rejection and success with no legacy
rendered NTTP map.

## Suggested Next

Continue Step 4 by tracing another parser-to-Sema handoff family, preferably
ordinary known-function call references or NTTP default expression references,
and either repair one concrete metadata drop or record the exact missing
carrier.

## Watchouts

- The source idea remains active; Step 3.3 closure is not source-idea closure.
- Step 4 should preserve structured metadata that already exists. Do not add
  fallback spelling, rendered qualified-text parsing, helper-only renames, or
  string/string_view semantic lookup routes.
- The new forwarded NTTP carrier covers parser/Sema AST call-env binding for
  simple forwarded NTTP names, including environments where only structured
  text metadata is present. It does not claim cleanup of expression-string
  `$expr:` template arguments or HIR `NttpBindings` string compatibility.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- `review/step33_final_route_review.md` and prior review artifacts are
  transient review files and were not modified by this lifecycle update.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`.

Result: build succeeded and the filtered CTest subset passed
`885/885` tests. Fresh proof log: `test_after.log`.
