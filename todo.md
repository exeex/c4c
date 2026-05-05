# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Closed `ideas/open/144_member_template_constructor_specialization_metadata.md`
as `ideas/closed/144_member_template_constructor_specialization_metadata.md`
and resumed the parent TypeSpec-tag deletion plan at Step 6.

The decomposition close gate accepted the canonical positive-Sema regression
guard:

- `test_before.log`: 883/884, with only
  `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
  failing.
- `test_after.log`: 884/884, no failures.
- Guard result: pass, no new failing tests.

The red full-suite candidate from the decomposition route had 24 known failures
and was rejected as uncanonical. It is not close proof for the decomposition
and is not acceptance proof for the parent route.

## Suggested Next

Execute parent Step 6 broad-validation / remaining full-suite triage. Start by
establishing a fresh supervisor-selected canonical baseline for the parent
TypeSpec-tag deletion route, then continue deleting `TypeSpec::tag` and split
any unrelated validation boundary into a separate open idea.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Do not rely on the rejected red full-suite candidate as canonical proof.
- The constructor member-template specialization blocker is closed; do not
  reopen it unless a fresh parent deletion probe exposes a distinct regression
  in the same carrier.
- Split unrelated LIR/BIR/backend/codegen or broad-validation boundaries into
  follow-up open ideas rather than broadening this Step 6 packet.

## Proof

Close proof for the decomposition idea:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: passed. Before: 883/884. After: 884/884. Resolved failure:
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`.

No build or full-suite validation was run by the plan owner during this
lifecycle-only close/resume operation.
