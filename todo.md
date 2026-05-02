# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared/classified the
`src/frontend/hir/hir_functions.cpp` callable compatibility deletion blocker:

- Removed the generic `compatibility_type_tag()` helper that directly returned
  rendered `TypeSpec::tag`.
- Split the retained rendered-name reads into explicit legacy bridges:
  template binding only when no usable TextId spelling is available, member
  typedef owner spelling only when qualified TextId spelling is unavailable,
  and callable zero-sized-return struct lookup only when complete structured
  owner metadata is absent or incomplete.
- Kept structured `HirRecordOwnerKey` lookup authoritative for callable
  zero-sized-return normalization; a complete structured miss still blocks the
  rendered fallback.
- No focused test was added because `hir_functions.cpp` is staged in the build
  and existing `cpp_hir_template_function_pack_signature_binding` coverage
  exercises the affected legacy parameter-pack bridge.

## Suggested Next

Continue Step 4 with one bounded frontend/HIR packet for the next fresh
deletion-probe cluster, likely `src/frontend/hir/hir_types.cpp` lookup/payload
surfaces, per the supervisor's subset choice.

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
- `hir_functions.cpp` callable scaffolding no longer has the
  `compatibility_type_tag()` direct rendered-tag helper; retained rendered
  reads are classified as no-complete/no-usable-metadata compatibility bridges.
- `hir_types.cpp` still contains both semantic-looking lookup reads and
  compatibility/final-spelling payload writes. Split those before editing so
  ABI/link-visible names, diagnostics, dumps, and base-tag storage are not
  accidentally collapsed into owner-key semantics.
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
