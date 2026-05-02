# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries reran the controlled
`TypeSpec::tag` deletion probe after the recent HIR compatibility slices:

- Temporarily removed only `TypeSpec::tag` from
  `src/frontend/parser/ast.hpp`, ran `cmake --build --preset default`, and
  saved the expected failed probe output to
  `/tmp/c4c_typespec_tag_deletion_probe_after_hir_compat.log`.
- Restored `src/frontend/parser/ast.hpp` to the pre-probe state before running
  proof; `git diff -- src/frontend/parser/ast.hpp` was empty after restore.
- Fresh first-failure inventory remains in frontend/HIR compile blockers:
  `hir_functions.cpp`, `hir_lowering_core.cpp`, `hir_build.cpp`,
  `hir_types.cpp`, and `hir/impl/expr/call.cpp`. The probe still does not reach
  LIR/BIR/backend failures.
- `hir_functions.cpp` first fails in legacy template/type binding-name helpers
  and member-typedef metadata presence checks. This is a small compatibility
  helper cluster around no-usable/no-complete text metadata.
- `hir_lowering_core.cpp` and `hir_build.cpp` first fail only in already
  classified no-complete-metadata compatibility fallbacks for generic record
  compatibility, local layout lookup, and ref-overload grouping.
- `hir_types.cpp` still has several independent rendered-tag surfaces:
  typedef-to-struct conversion payload/fallback, struct-method lookup parity and
  owner fallback, member-symbol fallback, base final-spelling storage, and
  generic control-type template/operator-call paths.
- `hir/impl/expr/call.cpp` remains a separate call-lowering cluster covering
  template struct calls, temporary initialization type comparisons,
  pack-forward template argument checks, operator-call gating, implicit `this`
  scaffolding, and consteval template argument substitution.

## Suggested Next

Continue Step 4 with one bounded frontend/HIR packet focused on
`src/frontend/hir/hir_functions.cpp` lines 35-52: replace the direct rendered
`TypeSpec::tag` reads in the legacy template/type binding-name helpers and
member-typedef metadata presence check with TextId/owner-metadata-first helpers,
retaining rendered spelling only as an explicit no-usable/no-complete metadata
compatibility bridge.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- The callable zero-sized-return path should keep rendered `TypeSpec::tag`
  compatibility only when no complete structured owner metadata exists.
- The ref-overload grouping path now uses `tag_text_id` before rendered fallback
  when complete owner metadata is absent; its retained rendered tag comparison
  is only the named no-complete-metadata legacy bridge.
- The member-symbol lookup path now treats complete owner/member metadata as
  authoritative; stale rendered owner symbols are not a fallback after a
  complete miss, but `HirStructDef` fields remain a structured hit source when
  the owner lookup mirror is absent.
- The generic `NK_MEMBER` inference path now delegates owner lookup to the
  layout resolver; do not reintroduce direct `struct_defs[TypeSpec::tag]`
  lookup when complete `record_def` or TextId owner metadata is present.
- The typedef-to-struct resolver now treats complete `record_def` and TextId
  owner metadata as authoritative; keep rendered `TypeSpec::tag` fallback only
  for no-complete-metadata compatibility.
- `hir_lowering_core.cpp` still has two intentional rendered `TypeSpec::tag`
  reads: generic record compatibility and local layout TypeSpec lookup. Both
  are classified no-complete-metadata compatibility bridges, not semantic
  owner keys.
- The base layout path still uses `HirStructDef::base_tags` as final spelling
  and as no-complete-metadata compatibility input; it no longer writes
  `TypeSpec::tag`.
- `hir_functions.cpp` still has the current next bounded cluster: no-usable
  template parameter spelling fallback, no-complete tag-text fallback, and
  member-typedef owner metadata-or-legacy-name detection.
- `hir_types.cpp` layout/member-base helpers now classify retained rendered
  lookup as no-complete-metadata compatibility. Base traversal should continue
  to prefer complete `base_tag_text_ids` owner metadata before rendered
  `base_tags`.
- `hir_lowering_core.cpp` and `hir_build.cpp` retained failures are already
  classified fallback helpers. They may be mechanical after the
  `hir_functions.cpp` helper pattern is clarified, but should stay separate
  unless the supervisor deliberately groups compatibility-helper cleanup.
- `hir_types.cpp` still contains lookup, parity, compatibility, and
  final-spelling payload surfaces outside the cleared layout/member-base helper
  family. Split those before editing so ABI/link-visible names, diagnostics,
  dumps, and base-tag storage are not accidentally collapsed into owner-key
  semantics.
- `hir/impl/expr/call.cpp` remains a later first-wave HIR cluster after the
  top-level HIR files compile.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Do not create downstream follow-up ideas until a probe reaches LIR/BIR/backend
  failures after frontend/HIR compile blockers are cleared.
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
- `hir_lowering_core.cpp` retained rendered-tag fallback reads only for
  no-complete-metadata compatibility; do not treat those as semantic authority.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved as the canonical
executor proof log. The build passed; CTest ran 73 HIR tests and all passed.

Non-canonical probe artifact:
`/tmp/c4c_typespec_tag_deletion_probe_after_hir_compat.log`
