# Shared StructNameId Table For LIR Type Identity

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md](/workspaces/c4c/ideas/closed/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md)

## Goal

Introduce a shared `StructNameId` / `StructNameTable` facility and make LIR plus
HIR-to-LIR use it for LLVM struct type identity.

This is the first required step before broader LIR type cleanup. The immediate
target is not to model C++ record semantics in LIR. The target is to give LIR a
cheap, text-table-backed identity for LLVM struct type names such as:

```llvm
%struct.Node = type { i32, ptr }
```

The migration must use the same dual-track discipline as the parser, sema, and
HIR lookup migrations: introduce the structured path beside the existing
rendered string path, prove parity, and only remove legacy rendered strings in
later follow-up work.

## Why This Idea Exists

Function and global symbols already have `LinkNameId` because they are
link-visible identities. Struct/class/template-class lowering is different:
after HIR-to-LIR, there should be no C++ class, inheritance, method ownership,
or template semantic identity left. Those high-level facts should already have
been erased into layout.

However, LIR still needs to name and reference LLVM struct layout declarations.
Today that role is still mostly played by rendered strings derived from
`TypeSpec::tag` and `Module::struct_defs`. That keeps semantic and emitted type
spelling tangled together.

The clean next step is a small shared table, similar in spirit to
`LinkNameTable`, but for LLVM struct type names:

```text
StructNameId -> TextTable spelling -> "%struct.X" printer output
StructNameId -> LIR struct declaration fields/layout
```

The first slice should not switch emission wholesale to the new table. It
should dual-write structured declarations beside the existing rendered
`type_decls`, keep current printer output stable, and add a sanity/parity route
that can compare structured rendering against the legacy text before any legacy
cleanup is attempted.

## Design Direction

Add a shared header under `src/shared/`, for example:

```text
src/shared/struct_name_table.hpp
```

The shared layer should define:

```cpp
using StructNameId = uint32_t;
constexpr StructNameId kInvalidStructName = 0;
using StructNameTable = SemanticNameTable<StructNameId, kInvalidStructName>;
```

It should reuse the existing `TextTable` / `SemanticNameTable` machinery so
lookup remains cheap and later stages can get `len + const char*` from the
shared text storage.

## LIR Model Direction

LIR should gain a small struct declaration model such as:

```cpp
struct LirStructField {
  LirTypeRef type;
};

struct LirStructDecl {
  StructNameId name_id = kInvalidStructName;
  std::vector<LirStructField> fields;
};
```

`LirModule` should carry:

```cpp
StructNameTable struct_names;
std::vector<LirStructDecl> struct_decls;
std::unordered_map<StructNameId, size_t> struct_decl_index;
```

The existing `type_decls` rendered string vector should remain as compatibility
or printer fallback during the first slice.

The first implementation should treat `type_decls` as the legacy authority for
emission and `struct_decls` as a structured mirror. Later ideas may demote
`type_decls` only after parity proof is stable.

## HIR-To-LIR Boundary Direction

HIR-to-LIR should be responsible for erasing high-level record information into
LIR layout identity:

```text
HIR HirStructDef / TypeSpec::tag / template instantiation tag
  -> StructNameId
  -> LirStructDecl fields
  -> printer renders %struct.name = type { ... }
```

At LIR level there is no inheritance concept. Any ordinary struct, class,
template class/struct instantiation, or inherited layout should already have
been lowered into field layout before or during HIR-to-LIR.

## Out Of Scope

- Modeling C++ record ownership in LIR.
- Carrying `HirRecordOwnerKey` as the primary LIR type identity.
- Removing all `TypeSpec::tag` uses in HIR.
- Removing rendered `type_decls` in the first slice.
- Making structured struct declarations the sole printer authority in the
  first slice.
- Rewriting all `LirTypeRef` type strings at once.
- Changing emitted LLVM type names or ABI/link-visible symbol names.

## Dual-Track Migration Rules

- Always keep the existing rendered `type_decls` path during this idea.
- Dual-write a structured `StructNameId -> LirStructDecl` mirror when a rendered
  type declaration is produced.
- Prefer current printer behavior until structured rendering has exact parity.
- Add mismatch observation or a shadow-render helper so sanity checks can prove
  structured output matches legacy output.
- Do not delete or demote legacy strings as part of this idea.
- Record any remaining `TypeSpec::tag` or rendered-name dependency as follow-up
  cleanup, not as a blocker for the first structured mirror.

## Suggested Execution Decomposition

1. Add `src/shared/struct_name_table.hpp` with `StructNameId`,
   `kInvalidStructName`, and `StructNameTable`.
2. Attach `StructNameTable` to `LirModule` using the existing shared
   `TextTable`.
3. Add a minimal `LirStructDecl` / `LirStructField` representation beside
   existing rendered `type_decls`.
4. During `build_type_decls` or nearby HIR-to-LIR lowering, intern each LLVM
   struct type name into `StructNameTable` and dual-write a structured
   declaration beside the rendered text line.
5. Add lookup helpers from `StructNameId` to structured fields/layout.
6. Add a structured shadow-render or parity helper that can compare the
   structured declaration text against the legacy rendered `type_decls` entry.
7. Keep printer output stable by rendering the existing text path first.
8. Add focused proof for ordinary structs, template struct/class
   instantiations, nested struct fields, and layout-sensitive codegen.

## Acceptance Criteria

- `StructNameId` and `StructNameTable` exist in `src/shared/`.
- LIR owns a `StructNameTable` sharing the module text table.
- HIR-to-LIR interns LLVM struct type names into `StructNameId`.
- LIR has a structured `StructNameId -> fields/layout` declaration path beside
  existing rendered `type_decls`.
- Structured declarations and legacy rendered `type_decls` are dual-written.
- A sanity/parity check or shadow-render path can prove structured declaration
  rendering matches legacy output.
- The existing printer still emits through the legacy path unless exact parity
  has been proven and explicitly switched in a later idea.
- Existing LLVM output remains unchanged.
- `TypeSpec::tag` is no longer the only route available for LIR struct type
  declaration identity.
