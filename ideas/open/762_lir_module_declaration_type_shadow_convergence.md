# LIR module declaration type shadow convergence

## Intent

After call/signature type mirrors are converged, retire module-level type text
as semantic authority for extern declarations, function signatures, globals,
and struct declarations.

This idea depends on:

- `ideas/open/759_lir_typed_ref_enum_foundation.md`
- `ideas/open/760_lir_string_constructor_deprecation_migration.md`
- the planned call/signature convergence idea
  `ideas/open/761_lir_call_signature_type_mirror_convergence.md`

It should run after 761 has made call and signature type refs authoritative at
the instruction/signature boundary.

## Why This Exists

`src/codegen/lir/ir.hpp` already contains structured mirrors for module-level
identity:

- extern declarations have `LinkNameId` and `LirTypeRef` return types
- functions have `LinkNameId`, `TypeSpec`, `signature_params`, and
  `signature_*_type_ref` fields
- globals have `LinkNameId`, `TypeSpec`, and optional `llvm_type_ref`
- struct declarations have `StructNameId` and `LirStructDecl`

But several legacy rendered fields remain close enough to semantic consumers
that they can still be mistaken for authority:

- `LirExternDecl::return_type_str`
- `LirModule::ExternDeclInfo::return_type_str`
- `LirFunction::signature_text`
- `LirGlobal::llvm_type` beside `llvm_type_ref`
- `LirModule::type_decls` beside `struct_decls`

Those strings should become printer/emission shadows or compatibility payloads.
The structured ids and refs should be the module-level source of truth.

## In Scope

- Audit extern declaration recording, function signature publication, global
  type publication, struct declaration recording, verifier checks, printer use,
  and backend consumers.
- Make `LirExternDecl::return_type` and `ExternDeclInfo::return_type` the
  authoritative return type carriers; `return_type_str` should become rendered
  shadow or be removed when safe.
- Make `LirFunction` structured signature fields the authority over
  `signature_text` for verifier/backend behavior.
- Make `LirGlobal::llvm_type_ref` or a documented structured replacement the
  authority over `llvm_type` when exact type identity is available.
- Make `LirModule::struct_decls` and `StructNameId` the authority over
  `type_decls`; retain `type_decls` only as a legacy shadow until all emitters
  are structured.
- Preserve final LLVM output by rendering from structured state where possible.
- Add focused coverage for extern return types, function signatures, globals,
  and struct declarations with misleading or stale shadow text.

## Out Of Scope

- Replacing all aggregate/vector/struct/function type spellings with a full
  typed type tree.
- Rewriting HIR `TypeSpec` lowering wholesale.
- Changing global initializer semantics beyond preserving function-reference
  identity that already has `initializer_function_link_name_ids`.
- Changing data layout strings, raw string literal bytes, inline assembly text,
  or template specialization metadata.
- Raw-BIR receiver work, target lowering, MIR, or broad printer rewrites not
  needed to render from structured module-level type state.
- Removing compatibility shadows before their producer and consumer routes have
  explicit structured replacements.

## Acceptance Criteria

- Extern declaration dedup and verification use `LinkNameId` plus structured
  return type refs rather than raw name/return type text when ids are present.
- Function signature verification/backend paths prefer `signature_params`,
  `signature_return_type_ref`, and `signature_param_type_refs` over
  `signature_text`.
- Global type consumers prefer `llvm_type_ref` or a deliberate structured
  replacement over `llvm_type` when exact type identity exists.
- Struct declaration consumers prefer `struct_decls` and `StructNameId` over
  `type_decls`; legacy `type_decls` is explicitly compatibility-only.
- Focused tests prove stale or misleading module-level shadow text cannot
  override structured ids and type refs.
- The repository builds and selected LIR/frontend/backend tests for externs,
  function signatures, globals, and struct declarations pass.

## Suggested Starting Points

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/verify.cpp`
- `src/codegen/lir/print.cpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- extern declaration and global/type-ref frontend tests
- backend structured context and LIR-to-BIR interface tests that consume
  module-level declarations

## Reviewer Reject Signals

- The patch continues to deduplicate or verify module declarations by raw
  rendered type text when structured ids and refs are available.
- `signature_text`, `llvm_type`, or `type_decls` remains an undocumented
  semantic fallback rather than an explicit compatibility/emission shadow.
- Compatibility shadows are removed before current emitters, verifiers, and
  backend consumers can render or consume equivalent structured state.
- The change broadens into Raw-BIR, target lowering, MIR, or unrelated value
  identity work.
- Tests are weakened, expectations are downgraded, or verifier rules are
  relaxed to make stale shadow text pass.
- The result still allows stale module-level type strings to override
  `LirTypeRef`, `StructNameId`, or `LinkNameId` authority.
