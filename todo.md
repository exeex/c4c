# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.1
Current Step Title: Inventory Live Member-Typedef Mirror Consumers

## Just Finished

Step 2.3 is lifecycle-exhausted after code review and proof repair.

Reviewer report `review/step2_3_post_render_reentry_review.md` found the
Step 2.3 implementation aligned with
`ideas/open/139_parser_sema_rendered_string_lookup_removal.md`, found no
testcase-overfit route, and identified only proof bookkeeping as the blocker.
The blocker has been repaired with a fresh canonical `test_after.log` for:

`(cmake --build build --target frontend_parser_lookup_authority_tests && ctest --test-dir build -R '^frontend_parser_lookup_authority_tests$' --output-on-failure) > test_after.log 2>&1`

Regression guard was reported passing with `--allow-non-decreasing-passed`.
No named remaining Step 2.3 implementation packet is recorded; do not keep
Step 2.3 active for grep-driven cleanup.

## Suggested Next

Start Step 2.4.1: inventory the live `owner::member` typedef mirror consumers
before deleting or shrinking mirror storage.

The next executor packet should classify each live mirror writer and reader by
the strongest available structured carrier: `TypeSpec::record_def`, direct
record/member metadata, member typedef arrays, namespace context id,
`QualifiedNameKey`, template-instantiation metadata, or missing metadata. It
should record the smallest next conversion or metadata blocker in this file and
must not claim code proof for inventory-only lifecycle work.

## Watchouts

- Do not invent another Step 2.3 packet without a named concrete parser/Sema
  rendered semantic authority route.
- Step 2.4.1 is inventory-first. Do not delete the `owner::member` mirror,
  delete APIs, or claim storage narrowing until live consumers and missing
  carriers are classified.
- Explicitly reject helpers that accept rendered `owner::member` text,
  `std::string`, or `std::string_view` qualified text and parse it back into
  owner/member identity.
- The known qualified member typedef blocker from Step 2.3 remains relevant:
  instantiated member typedefs such as `ns::holder_T_int::type` need concrete
  owner-member `TextId` or equivalent domain metadata. Full `TypeSpec::tag`
  field deletion belongs to idea 141, not this packet.
- HIR-only rendered lookup cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

Passed:
`(cmake --build build --target frontend_parser_lookup_authority_tests && ctest --test-dir build -R '^frontend_parser_lookup_authority_tests$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log` (1 focused CTest passed, including the drifted
qualified typedef and dependent-typename visible-type rendered-spelling
re-entry coverage).

Review accepted after proof repair:
`review/step2_3_post_render_reentry_review.md`.
