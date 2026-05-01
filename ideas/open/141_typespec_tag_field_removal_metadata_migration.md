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
