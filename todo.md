# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries reran the controlled
`TypeSpec::tag` deletion probe and cleared the next first-failing
`src/frontend/hir/hir_lowering_core.cpp` cluster. Generic selection record
compatibility now uses complete HIR owner keys, parser record-def identity, or
parser TextId identity instead of comparing rendered `TypeSpec::tag` spelling.
The local layout probe helper now requires structured owner metadata for
TypeSpec-backed struct/union layout lookup instead of falling back to the
rendered `struct_defs[TypeSpec::tag]` bridge.

## Suggested Next

Continue Step 4 by taking the next first deletion-probe blocker in
`src/frontend/hir/hir_build.cpp`:
`ref_overload_record_types_match_without_complete_owner_key` still compares
rendered `TypeSpec::tag` spelling when complete owner metadata is absent.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- `hir_functions.cpp` no longer directly reads `TypeSpec::tag` in the targeted
  helper/member-typedef cluster.
- Qualified dependent member typedefs with a unique template owner/member are
  now resolved structurally from parser TextIds and current template bindings;
  keep `member_typedef_text_ids` as the member carrier and do not replace that
  with rendered owner-name lookup.
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
- `hir_lowering_core.cpp` no longer has direct `TypeSpec::tag` reads in generic
  record compatibility or local layout TypeSpec lookup.
- The base layout path still uses `HirStructDef::base_tags` as final spelling
  and as no-complete-metadata compatibility input; it no longer writes
  `TypeSpec::tag`.
- The former `hir_functions.cpp` lines 35-52 deletion-probe cluster is cleared
  of direct rendered tag reads.
- `hir_types.cpp` layout/member-base helpers now classify retained rendered
  lookup as no-complete-metadata compatibility. Base traversal should continue
  to prefer complete `base_tag_text_ids` owner metadata before rendered
  `base_tags`.
- The `hir_build.cpp` retained ref-overload failure is already classified as a
  fallback helper. It should stay separate unless the supervisor deliberately
  groups compatibility-helper cleanup.
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
- The `hir_lowering_core.cpp` deletion-probe cluster is now clear of direct
  `TypeSpec::tag` reads. The first residual probe blocker moved to
  `src/frontend/hir/hir_build.cpp`, followed by remaining `hir_types.cpp` and
  `hir/impl/expr/call.cpp` direct-tag clusters.
- Non-canonical deletion probe artifacts for this packet:
  `/tmp/c4c_typespec_tag_deletion_probe_step4_next.log` and
  `/tmp/c4c_typespec_tag_deletion_probe_step4_after_hir_lowering_core.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved as the canonical
executor proof log. The build passed, and CTest passed 73 of 73 focused HIR
tests.
