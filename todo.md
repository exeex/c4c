# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair the targeted warning-inventory route

## Just Finished

- Step 6 targeted warning-inventory repair complete: classified the lvalue
  indexed-GEP element-type family. Its closed void-pointee `"i8"` fallback now
  uses `LirBuiltinType::I8`; remaining HIR-rendered element text flows only
  through the deprecated, searchable local
  `hir_rendered_indexed_gep_element_type_text` runtime-text helper. Existing
  structured `struct_type`/`union_type` paths remain unchanged.

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
- HIR aggregate, field, and signature fallback text may lack `StructNameId` or
  represent array-backed fields; preserve it through its dedicated boundary
  without changing structured aggregate paths or the AArch64 vector ABI cast.
- Indexed-GEP element text is HIR-rendered and may legitimately be array,
  pointer, vector, or other non-builtin text; retain the local lvalue helper
  while preserving its structured aggregate path.

## Proof

- `cmake --build --preset default` succeeded. Its warnings named only
  deliberate local inventory boundaries, including the indexed-GEP helper.
- `ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_global_type_ref|frontend_lir_function_signature_type_ref)$' > test_after.log`
  passed (2/2); `test_after.log` is the focused proof log.
