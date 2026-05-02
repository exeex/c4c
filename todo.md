# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the bounded
frontend/HIR layout cluster in `src/frontend/hir/hir_lowering_core.cpp`.

`generic_type_compatible` now compares complete structured record owner keys
before rendered tags, and `compute_struct_layout` field/base sizing now resolves
record layouts through complete `record_def`/`tag_text_id` owner metadata before
the explicit no-metadata rendered fallback. A complete structured owner-key miss
returns the default unknown-record layout instead of consulting a stale rendered
`TypeSpec::tag`. Focused `frontend_hir_lookup_tests` coverage now proves
standalone struct layout field size/alignment prefer the structured owner and
reject stale rendered tags after a complete structured miss.

## Suggested Next

Continue Step 4 with the next bounded frontend/HIR deletion-probe cluster from
the remaining inventory. The callable helper blocker in `hir_functions.cpp` and
the standalone layout helper cluster in `hir_lowering_core.cpp` are cleared;
remaining probe inventory still includes parser-owned semantic
producers/consumers and first-failure HIR clusters in
`src/frontend/hir/impl/compile_time/compile_time_engine.hpp`,
`src/frontend/hir/hir_types.cpp`, `src/frontend/hir/hir_ir.hpp`,
`src/frontend/hir/impl/expr/builtin.cpp`, and
`src/frontend/hir/hir_build.cpp`.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- The callable zero-sized-return path should keep rendered `TypeSpec::tag`
  compatibility only when no complete structured owner metadata exists.
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
- `hir_lowering_core.cpp` still intentionally contains rendered-tag fallback
  reads for no-metadata compatibility; do not treat those as semantic authority
  unless the structured owner-key probe lacks complete metadata.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved as the canonical
executor proof log. The build passed; CTest ran 73 HIR tests and all passed.
