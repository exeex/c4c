# TypeSpec Tag Field Removal Metadata Migration

Status: Open
Created: 2026-05-01

Parent Ideas:
- `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md`

## Goal

Remove `TypeSpec::tag` as a cross-stage semantic identity field.

`TypeSpec` should carry semantic type identity through `TextId`, record
definition pointers, declaration/owner objects, namespace context, structured
record/template/member keys, HIR record owner keys, or downstream type refs.
Rendered names may remain only as final spelling, diagnostics, dumps, mangling,
ABI/link-visible text, or explicitly classified no-metadata compatibility
payloads outside `TypeSpec::tag`.

## Why This Idea Exists

An experiment on 2026-05-01 deleted `const char* tag` from `TypeSpec` and ran:

```sh
cmake --build build --target c4cll
```

The build failed immediately because parser, HIR, and lowering still read
`TypeSpec::tag` as struct/union/enum/typedef identity, template parameter
identity, record lookup key, and display spelling. The first large failure
clusters were:

- `src/frontend/hir/impl/compile_time/compile_time_engine.hpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/hir_lowering_core.cpp`
- `src/frontend/hir/impl/expr/builtin.cpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/parser/ast.hpp`

This idea exists so `TypeSpec::tag` removal is handled as an explicit metadata
migration instead of being hidden inside parser/Sema string cleanup or HIR
rendered lookup resweeps.

## Scope

- Inventory every `TypeSpec::tag` read/write and classify it as semantic
  identity, final/display spelling, diagnostics/dumps, mangling/link spelling,
  compatibility payload, or missing metadata.
- Treat `TypeSpec::tag` deletion and cross-stage fallout as deferred from idea
  139. Parser/Sema cleanup may convert individual call sites to existing
  `tag_text_id`, `record_def`, or structured owner/member/template metadata,
  but deleting the field is owned by this idea.
- Replace parser-owned semantic `TypeSpec::tag` use with existing
  `tag_text_id`, `record_def`, `template_param_*`, `deferred_member_*`, or
  structured owner/member/template carriers where those are already available.
- Add or preserve producer metadata when a `TypeSpec` currently carries only a
  rendered tag but the producer has a real `TextId`, record definition, owner
  key, template parameter index, or concrete instantiated member identity.
- Migrate HIR consumers that use `TypeSpec::tag` as semantic lookup authority
  to HIR/module structured carriers such as declaration ids, record owner keys,
  `TextId`, `LinkNameId`, or module type refs.
- Split downstream LIR/BIR/backend carrier gaps into follow-up open ideas when
  removing `TypeSpec::tag` exposes missing non-HIR metadata.
- Add focused tests where stale or drifted rendered `TypeSpec` spelling must
  not decide semantic lookup once structured metadata exists.

## Deferred From Idea 139 And The Field-Removal Experiment

The 2026-05-01 build experiment showed that removing `TypeSpec::tag` is larger
than parser/Sema rendered lookup cleanup. This idea owns the remaining field
removal route, including:

- Parser-owned semantic `TypeSpec::tag` call sites that still need to be
  migrated to `tag_text_id`, `record_def`, `QualifiedNameKey`,
  template-parameter metadata, deferred member metadata, or another domain key.
- HIR consumers that use `TypeSpec::tag` as struct/union/enum/typedef
  identity, template parameter identity, record lookup key, member lookup key,
  or display/source spelling without a separated semantic carrier.
- HIR lowering and compile-time paths in the first failing build clusters:
  `compile_time_engine.hpp`, `hir_types.cpp`, `hir_ir.hpp`,
  `hir_functions.cpp`, `hir_lowering_core.cpp`, `builtin.cpp`, and
  `hir_build.cpp`.
- Any downstream LIR/BIR/backend carrier gap exposed after HIR no longer reads
  semantic identity from `TypeSpec::tag`; those gaps should become separate
  open ideas rather than resurrecting a rendered semantic field on `TypeSpec`.

## Lifecycle Checkpoints

- 2026-05-05: Parked again before continuing field deletion. The broad
  validation failures reviewed in
  `review/139_140_141_failure_attribution_review.md` and the follow-up fixes
  since `119a6ef0430fbb4967cfd79f7d6b3a12ad5f6bc8` show a shared upstream
  route problem: raw `TypeSpec::tag_text_id` and rendered tag compatibility are
  being canonicalized tentatively at many parser/Sema/HIR/codegen consumer
  sites instead of once at TypeSpec production and parser-to-HIR handoff
  boundaries. Do not continue deleting `TypeSpec::tag` under this parent
  runbook until the separate normalization initiative
  `ideas/open/143_typespec_identity_normalization_boundary.md` has established
  a stable normalized identity contract.
- 2026-05-03: Frontend/parser/Sema/HIR-owned deletion-probe residuals were
  cleared through Step 4. The next first failure is a downstream codegen/LIR
  carrier boundary at `src/codegen/shared/llvm_helpers.hpp:444`, with related
  `src/codegen` aggregate type-name, layout, field-chain, call, va_arg, and
  ABI lookup sites still reading `TypeSpec::tag`. This source idea is parked
  rather than closed because `TypeSpec::tag` still exists and the field-removal
  build is blocked by downstream carrier work. Follow-up idea
  `ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md` owns that
  downstream boundary.
- 2026-05-03: Follow-up idea
  `ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md` cleared the
  codegen/LIR aggregate identity deletion-probe boundary and was retired
  parked, not closed. The next deletion-probe residuals are frontend/HIR test
  fixtures that still directly reference `TypeSpec::tag`, so this parent idea
  resumes ownership of field removal and fixture cleanup. Do not reactivate
  idea 142 for parser/HIR test-fixture residuals unless a fresh parent probe
  exposes a new codegen/LIR aggregate identity boundary.

## Out Of Scope

- Removing final emitted names, diagnostics, dump text, ABI/link spelling, or
  mangled names merely because they are strings.
- Replacing `TypeSpec::tag` with another rendered-string field that remains
  semantic lookup authority.
- Broad LIR, BIR, backend, or codegen rewrites except to create follow-up
  metadata ideas when the frontend/HIR migration exposes a boundary.
- Weakening tests, marking supported cases unsupported, or introducing
  testcase-shaped shortcuts.

## Acceptance Criteria

- `TypeSpec` no longer has a `const char* tag` field.
- Parser-owned semantic type identity previously stored in `TypeSpec::tag` is
  represented by `TextId`, `record_def`, template parameter metadata, deferred
  member metadata, `QualifiedNameKey`, or another domain key.
- HIR no longer uses rendered `TypeSpec` spelling as semantic lookup authority
  for metadata-backed struct/union/enum/typedef/template/member routes.
- Any retained rendered spelling connected to type specs is clearly outside
  `TypeSpec::tag` and classified as diagnostics, display, dumps, mangling,
  ABI/link text, final output, or explicit no-metadata compatibility.
- A `cmake --build build --target c4cll` build passes after the field is
  removed.
- Focused frontend/HIR tests prove structured metadata wins over stale rendered
  type spelling for covered routes.

## Reviewer Reject Signals

- The change keeps semantic lookup alive by replacing `TypeSpec::tag` with a
  differently named rendered-string field or helper.
- A route claims `TypeSpec::tag` removal while continuing to recover semantic
  identity from rendered spelling after `TextId`, `record_def`, owner/member,
  template, or HIR structured metadata exists.
- Parser/HIR tests are weakened, marked unsupported, or rewritten around one
  named fixture instead of proving same-feature structured-vs-rendered
  disagreement.
- HIR, LIR, BIR, or backend gaps are silently worked around with string
  rediscovery instead of receiving structured carriers or separate open ideas.
- The implementation removes diagnostic, dump, ABI/link, mangling, or final
  spelling output and presents that as semantic metadata progress.
- A build failure after deleting `TypeSpec::tag` is addressed by reintroducing
  a broad compatibility field on `TypeSpec` rather than migrating the specific
  producer/consumer contract.
