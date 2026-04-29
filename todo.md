# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Demote struct_tag_def_map To Compatibility Mirror

## Just Finished

Plan-owner reviewed Step 4 template record identity propagation and advanced
the runbook to Step 5.

Step 4 is complete for the currently identified template-only propagation
scope:
- direct template emission returns the created or selected record identity
- template struct specialization and injected fallback populate
  `TypeSpec::record_def`
- template base reconstruction carries record identity when a known record is
  available
- template substitution preserves record identity through bases, member
  typedefs, fields, method returns, and method parameters
- focused tests cover stale rendered tag resistance for those paths

## Suggested Next

Execute Step 5 first packet: inventory every remaining
`DefinitionState::struct_tag_def_map` read and write after the typed record
identity propagation work, classify each as one of:
- compatibility/final-spelling mirror
- typed-identity fallback for tag-only compatibility
- still-semantic lookup that should be converted, documented, or split

Keep the first packet bounded to classification plus the smallest safe
demotion/documentation change if an obvious local helper/comment target exists.
If any remaining reader is still primary semantic authority rather than
fallback compatibility, report it as a Step 5 blocker instead of renaming or
papering over it.

## Watchouts

Do not delete `struct_tag_def_map` or remove rendered template compatibility
keys during Step 5. Preserve `TypeSpec::tag` as spelling, diagnostics, emitted
text, and compatibility payload.

Do not infer semantic record identity from an existing rendered map entry when
a typed `record_def` producer should be carrying the identity directly. Stale
map-entry tests are the guardrail for this step.

## Proof

Delegated proof passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Additional local validation:

`git diff --check -- src/frontend/parser/impl/types/base.cpp tests/frontend/frontend_parser_tests.cpp todo.md test_after.log`

Proof log: `test_after.log`

Plan-owner decision: Step 4 complete; no template-only split needed before
Step 5.
