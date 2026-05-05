# TypeSpec Identity Normalization Boundary

Status: Closed
Created: 2026-05-05

Related Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/closed/140_hir_legacy_string_lookup_metadata_resweep.md`
- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
- `review/139_140_141_failure_attribution_review.md`

## Goal

Define and implement one stable normalization boundary for TypeSpec semantic
identity so parser, Sema, HIR, and codegen consumers stop rediscovering record,
typedef, owner, and tag identity through scattered tentative rendered-tag
lookups.

`TypeSpec::tag_text_id` must be treated as text-table-local identity until it
is normalized into a stage-appropriate domain key. Rendered tag strings may
remain final spelling, diagnostics, dumps, ABI/link text, or explicit
no-metadata compatibility, but they must not be the normal semantic lookup
repair path once structured metadata exists.

## Why This Idea Exists

Since commit `119a6ef0430fbb4967cfd79f7d6b3a12ad5f6bc8`, repairs have been
forced into multiple consumers:

- consteval record lookup canonicalized `TypeSpec` ids through
  `link_name_texts->find`
- codegen aggregate lookup preferred rendered C-string compatibility tags over
  unsafe `link_name_texts->lookup(ts.tag_text_id)`
- aggregate owner-key helpers learned to detect TextId-table collisions and
  rebuild HIR owner keys
- parser typedef resolution still had to preserve the source typedef
  `tag_text_id` over stale stored TypeSpec metadata

Those fixes reduce failures, but they are symptoms of a missing normalized
identity contract. The same question should not be answered independently in
`lookup_record_layout`, `StmtEmitter`, LIR helpers, parser disambiguation, and
EASTL/template owner resolution.

## In Scope

- Inventory the current consumer-side canonicalization and tentative lookup
  surfaces introduced or touched after
  `119a6ef0430fbb4967cfd79f7d6b3a12ad5f6bc8`.
- Define the normalization contract for TypeSpec producers:
  `record_def` where available, otherwise namespace-qualified `tag_text_id`,
  template parameter identity, deferred member identity, or another explicit
  domain key.
- Define the parser-to-HIR handoff contract that converts parser-local
  `TextId` values into HIR-module canonical identities such as
  `HirRecordOwnerKey`, `ModuleDeclLookupKey`, `LinkNameId`, or module type
  references.
- Centralize cross-table canonicalization in shared helpers instead of letting
  each consumer call `lookup`, `find`, rendered fallback, or owner-index scans
  independently.
- Migrate one failure family at a time away from consumer-side tentative
  rediscovery, starting with record/layout identity and then parser
  disambiguation/template owner identity.
- Preserve or add stale-rendered-spelling tests proving structured metadata
  wins after normalization.
- Keep `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
  parked until this contract is in place.

## Out Of Scope

- Deleting `TypeSpec::tag` itself; idea 141 resumes that once normalization is
  stable.
- Reopening broad parser/Sema or HIR cleanup as a generic rendered-string
  removal pass.
- LIR/BIR/backend rewrites unrelated to normalized aggregate/type identity.
- Removing diagnostics, dumps, source spelling, mangling, final output, or
  ABI/link-visible text merely because it is string-shaped.
- Weakening tests, marking supported cases unsupported, or changing expected
  output to hide an unresolved identity gap.

## Acceptance Criteria

- There is one documented and tested TypeSpec normalization helper or helper
  family used by the main record/type identity consumers.
- Metadata-backed consumers do not directly trust raw `TypeSpec::tag_text_id`
  against a different `TextTable`.
- Parser typedef/type producers preserve source identity at production time
  instead of requiring downstream stale-tag repair.
- HIR/codegen record layout lookup uses normalized owner keys or module type
  identity as the primary path; rendered tag lookup is explicit no-metadata
  compatibility only.
- The failure count from
  `review/139_140_141_failure_attribution_review.md` is reduced through
  semantic normalization rather than expectation rewrites.
- All failures attributed in
  `review/139_140_141_failure_attribution_review.md` are either fixed by
  semantic normalization or explicitly split into a narrower follow-up idea
  before this idea closes.
- This idea cannot close on a partial baseline. Final close requires
  `ctest --test-dir build -j 8 --output-on-failure` to pass with zero failures.

## Reviewer Reject Signals

- A slice adds another local `lookup -> rendered fallback -> find` sequence in a
  consumer instead of routing through the normalized identity contract.
- Raw `TypeSpec::tag_text_id` is compared against or looked up in a `TextTable`
  that the producer did not own, without canonicalization through the central
  helper.
- A rendered tag string becomes the normal semantic authority for a path that
  already has `record_def`, namespace-qualified `tag_text_id`,
  `HirRecordOwnerKey`, template owner metadata, or deferred member metadata.
- The change only renames fallback/legacy helpers or moves tentative lookup
  code without reducing consumer-side rediscovery.
- Tests are weakened, marked unsupported, or made named-case-specific instead
  of proving same-feature stale-rendered-spelling disagreement.
- The route claims progress by deleting `TypeSpec::tag` before proving the
  normalized identity boundary is stable.

## Closure

Closed: 2026-05-05

Closure proof:

```sh
ctest --test-dir build -j 8 --output-on-failure > test_after.log 2>&1
```

Result recorded by supervisor: 3023/3023 tests passed, zero failures.

Lifecycle decision: the TypeSpec normalization boundary is stable enough to
resume `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`.
No narrower child idea is required from Step 6.
