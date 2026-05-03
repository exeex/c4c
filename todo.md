# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the previous frontend parser fixture cluster after the
alias-of-alias handoff. The normal `frontend_parser_tests` build and ctest
passed after restoration from the temporary deletion probe, and the probe moved
to the next direct fixture boundary at
`tests/frontend/frontend_parser_tests.cpp:7132`.

## Suggested Next

Migrate the next Step 2 deletion-probe boundary in
`test_type_binding_equivalence_uses_deferred_member_text_id_authority`, starting
at `tests/frontend/frontend_parser_tests.cpp:7132`, without weakening the
deferred-member TextId equivalence assertions.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Keep the deferred-member TextId authority checks meaningful: matching member
  TextIds should tolerate stale rendered names, one-sided or mismatched TextIds
  should reject, and rendered compatibility should remain only for the explicit
  no-metadata case.
- `src/frontend/parser/ast.hpp` should only be changed for temporary deletion
  probes unless this becomes the accepted final deletion packet.

## Proof

Pending for this packet. Use the supervisor-selected focused build/test command
for `frontend_parser_tests`, then run a temporary `TypeSpec::tag` deletion probe
to record the next boundary.
