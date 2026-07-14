# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair the targeted warning-inventory route

## Just Finished

- Step 6 targeted warning-inventory repair complete: the AArch64 fixed-vector
  ABI bitcast now uses `LirBuiltinType::I32` for its closed target and the
  HIR-rendered vector source value type only through the deprecated,
  searchable `hir_rendered_aarch64_vector_abi_source_type_text` local helper.
  The ABI conversion remains unchanged; HFA GEP/store text was not touched.

## Suggested Next

- Step 6: select the next targeted warning-inventory family; keep each local
  runtime-text boundary limited to its documented source provenance.

## Watchouts

- Do not remove or globally silence string construction warnings.
- Do not begin a repository-wide migration or rewrite HIR `TypeSpec` lowering.
- The global `const char*` warning experiment was rejected because it emitted
  widespread unrelated warnings; the repaired route must remain local.
- Keep dynamic aggregate, vector, struct, function, and selected-authority
  paths supported through explicit local boundaries where appropriate.
- Do not weaken verifier behavior, tests, or expected output.
- Do not retry a globally deprecated `const char*` constructor: it produces
  broad warning noise despite source-compatible compilation.
- The parsed typed-call boundary intentionally covers text supplied by the
  parser and its legacy string-mirror fallback; do not treat it as evidence
  that the generic `LirTypeRef` constructors should be deprecated.
- The parsed typed-call return boundary is intentionally limited to
  `make_lir_call_op`; do not broaden it to unrelated return-type construction.
- Extern declaration return type text is stored/external payload and may be
  dynamic; retain its local compatibility boundary rather than guessing a
  builtin enum.
- Inline-assembly type text comes from HIR `TypeSpec` through `llvm_ty(...)`;
  retain its dedicated local boundary rather than extending it to ordinary
  return or switch lowering.
- Call-target HIR-rendered fallback and HFA lane type text is dynamic; retain
  it through `hir_rendered_call_target_type_text` while preserving aggregate
  structured authority and call-signature behavior.
- HIR aggregate, field, and signature fallback text may lack `StructNameId` or
  represent array-backed fields; preserve it through its dedicated boundary
  without changing structured aggregate paths or the AArch64 vector ABI cast.
- Indexed-GEP element text is HIR-rendered and may legitimately be array,
  pointer, vector, or other non-builtin text; retain the local lvalue helper
  while preserving its structured aggregate path.
- The AArch64 fixed-vector parameter ABI bitcast has a closed `i32` target and
  a HIR-rendered vector source; retain the source-only local boundary without
  widening it to HFA GEP/store text.

## Proof

- `cmake --build --preset default` succeeded; its warnings are deliberate
  local inventory boundaries, including the new AArch64 vector ABI source
  helper.
- `ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|backend_lir_)' > test_after.log`
  passed (6/6); `test_after.log` is the focused proof log.
