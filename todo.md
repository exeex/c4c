Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten `last_using_alias_name` Around Structured Alias Identity

# Current Packet

## Just Finished

Step 4 helper cleanup completed for `last_using_alias_name` active-context
handoff. Removed the unused string-only
`set_last_using_alias_name(std::string_view)` fallback setter from `parser.hpp`
after confirming the only live setter call is the structured
`set_last_using_alias_name(const QualifiedNameKey&)` path in using-alias
registration.

## Suggested Next

Step 4 appears exhausted from the executor side. Supervisor/plan-owner can
advance the active runbook to Step 5 or perform the required lifecycle review.

## Watchouts

- Do not remove rendered alias spelling passed to `register_typedef_binding(...)`
  in `parser_declarations.cpp`; that is compatibility bridge data for existing
  string-keyed maps, not the structured alias-template semantic key.
- `last_using_alias_name_text_id` is currently written by the structured setter
  but not read outside `last_using_alias_name_text()`. That makes the next code
  packet a helper cleanup, not a semantic lookup migration.
- `parser_types_base.cpp` and `parser_types_template.cpp` have no live Step 4
  handoff sites after the earlier template-struct cleanup.
- Do not rework alias-template storage beyond the Step 4 active-context
  handoff boundary.
- Do not reopen completed template-struct primary/specialization/instantiation
  lookup cleanup.
- `last_using_alias_name_text()` remains as a compatibility accessor, but no
  current parser source call site uses it after the string-only setter removal.

## Proof

Delegated proof passed:

`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

Proof log: `test_after.log`.
