# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired one ordinary known-function call/reference handoff gap.
Sema function, ref-overload, and C++ overload lookup now consult the parser's
`using_value_alias_target_*` structured carrier before rendered callee spelling.
Sema also mirrors qualified function declarations under the normalized
qualified `TextId` key shape used by that parser carrier, so a using-imported
function call can preserve target identity through parser-to-Sema without
rendered-name rediscovery. Focused lookup-authority coverage now proves a
drifted rendered callee spelling still resolves through the structured using
target key.

## Suggested Next

Continue Step 4 by tracing NTTP default expression references or another
ordinary call-reference variant that is not covered by `using_value_alias_target_*`,
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
- The known-function slice covers using-imported call references whose parser
  carrier is target base `TextId` plus target qualifier `TextId` metadata. It
  does not fix parser production of resolved namespace context ids for
  namespaced free-function declarations that are rendered as `ns::fn`; that
  should be treated as a separate parser/Sema carrier packet if needed.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- `review/step33_final_route_review.md` and prior review artifacts are
  transient review files and were not modified by this lifecycle update.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`.

Result: build succeeded and the filtered CTest subset passed
`885/885` tests. Fresh proof log: `test_after.log`.
