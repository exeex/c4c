Status: Active
Source Idea Path: ideas/open/173_aggregate_layout_identity_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select And Map One Layout Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting the local aggregate GEP layout lookup
boundary for the first structured-layout migration.

Selected consumer:
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
  `lookup_local_gep_layout_result()` / `lookup_local_gep_layout()`, used by
  local aggregate GEP byte-slice, child-index, repeated-array, and dynamic
  aggregate access resolution.

Files inspected:
- `ideas/open/173_aggregate_layout_identity_structured_boundary.md`
- `plan.md`
- `src/codegen/lir/ir.hpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/types.cpp`
- `src/backend/bir/lir_to_bir/aggregate.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- `tests/backend/backend_prepare_structured_context_test.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`

Mapping evidence:
- Upstream structured facts already exist as `LirModule::struct_decls`,
  `LirStructDecl::fields`, `LirStructDecl::is_packed`, `StructNameId`, and the
  backend `BackendStructuredLayoutTable` built by
  `build_backend_structured_layout_table()`.
- The legacy text bridge to isolate is the rendered `%struct...` type text plus
  `TypeDeclMap` built from `LirModule::type_decls`; local GEP currently passes
  both through `lookup_backend_aggregate_type_layout_result()`.
- The selected consumer affects offset-sensitive behavior: raw `i8` aggregate
  byte-slice GEPs, child-index projections, repeated aggregate extents, and
  dynamic local aggregate array/member access.
- Metadata-rich mismatch risk: a generated path can carry structured
  declarations while an equal or stale rendered `%struct...` spelling still
  reaches lookup; implementation must fail closed or preserve visible mismatch
  status instead of silently accepting legacy `type_decls` layout authority.

## Suggested Next

Implement Step 2 for the selected local GEP boundary: thread/require structured
layout identity at `lookup_local_gep_layout_result()` call sites for generated
metadata-rich inputs while keeping the `TypeDeclMap` fallback explicit for
legacy no-structured-layout fixtures.

## Watchouts

- Do not treat equal rendered struct/type spelling as sufficient identity for
  metadata-rich generated paths.
- Keep legacy/no-metadata compatibility explicit and isolated.
- Do not weaken layout/backend expectations or add testcase-shaped shortcuts.
- Avoid claiming the shared backend lookup is fully migrated just because this
  packet maps the local GEP consumer; globals, byval copies, call ABI, and other
  consumers remain separate boundaries unless the supervisor expands scope.

## Proof

No build/test execution required for this boundary-selection packet.

Targeted proof command family for implementation:
- Build/compile proof: `cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test`
- Focused CTest proof: `ctest --test-dir build -R 'backend_prepare_structured_context|backend_lir_to_bir_notes' --output-on-failure`
- Local GEP behavioral focus inside that family: structured-vs-legacy mismatch
  checks in `backend_prepare_structured_context` plus local aggregate raw/dynamic
  GEP cases in `backend_lir_to_bir_notes`.
