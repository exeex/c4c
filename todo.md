# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Plan-owner review accepted Step 3 - Migrate HIR Type And Record Consumers as
complete enough to advance after five HIR consumer migration slices through
HEAD `d381b65e8`:

- Deferred member typedef resolution now stops after a structured `record_def`
  owner path fails instead of falling through to stale rendered owner spelling.
- Function specialization TypeSpec argument matching uses `record_def` or
  complete namespace/`tag_text_id`/qualifier TextId identity before rendered
  tag compatibility.
- Repeated template type deduction consistency applies structured TypeSpec
  nominal matching before rendered tag comparison.
- Canonical template struct primary lookup resolves through declaration owner
  keys and `record_def` owner identity, and blocks rendered origin fallback
  when structured owner metadata is present but has no entry.
- Inherited static member evaluation recurses through base `record_def` before
  consulting rendered `base_ts.tag`, and blocks stale rendered base fallback
  after a structured base miss.

Step 3 focused tests cover positive structured wins and negative
structured-miss/no-fallback boundaries for record-def primary lookup, inherited
static member lookup, function specialization TypeSpec matching, repeated
deduction consistency, and deferred member typedef record-def miss rejection.

Residual HIR `TypeSpec::tag` classifications accepted for the Step 4 probe:

- Deferred member typedef resolution still uses rendered
  `resolve_struct_member_typedef_type(tag, ...)` for no-metadata compatibility
  and concrete realized template-origin traversal after origin materialization.
- Function specialization TypeSpec argument matching still uses rendered `tag`
  comparison only when both sides lack structured nominal metadata.
- `TypeBindings` map keys remain rendered template parameter names; migrating
  that boundary needs a separate structured template parameter binding packet
  if the deletion probe shows it is still required.
- Canonical template struct primary lookup keeps rendered primary-name fallback
  for declarations without owner-key metadata, and template struct
  specialization lookup by primary owner remains a rendered-fallback surface.
- Inherited static member evaluation still has no HIR owner-key map, so
  `tag_text_id` and `HirRecordOwnerKey` lookup are residual surfaces for a
  later helper/API packet if the removal probe exposes them.

## Suggested Next

Start Step 4 - Probe Field Removal And Split Boundaries. Temporarily delete or
disable `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, run
`cmake --build --preset default`, and record the first compile-failure clusters
by ownership and semantic category. Revert only the temporary probe edit before
ending the packet. Keep parser/HIR-owned failures in this runbook; create
separate `ideas/open/` follow-ups only for downstream LIR/BIR/backend metadata
gaps that outlive this frontend/HIR runbook.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Restore the worktree to a buildable state after recording probe failures.
- Classify each failure as parser/HIR-owned, compatibility/display/final
  spelling, or downstream metadata gap instead of fixing broad clusters inside
  the probe packet.
- Do not create new follow-up ideas for parser/HIR work that still belongs in
  this runbook.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Treat any `TypeSpec::tag` deletion build as temporary until Step 5.

## Proof

Step 3 delegated packets recorded focused proof with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Accepted normalized proof is currently in `test_before.log`; it shows 60/60
selected tests passed. `test_after.log` is absent because the accepted proof
was rolled forward before this lifecycle review.

Code review in `review/step3_typespec_tag_hir_route_review.md` reported no
blocking findings, no overfit, route on track, and no reset or split needed.

`git diff --check -- todo.md` passed after the plan-owner transition.
