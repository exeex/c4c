# LinkNameId Backend Symbol Authority

Status: Open
Created: 2026-05-11

Parent Ideas:
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- `ideas/closed/160_sema_canonical_symbol_template_key_authority.md`
- `ideas/closed/159_sema_consteval_domain_table_authority.md`

## Goal

Make final link-visible symbol identity flow through `LinkNameId` from HIR into
LIR, BIR, and backend preparation whenever the compiler has complete symbol
metadata.

Rendered function/global names, template `mangled_name` strings, LLVM spelling,
and dump text may remain as output and compatibility payloads. They should not
be the primary identity for direct calls, global references, extern
declaration dedup, initializer symbol references, or backend symbol lookup
when a `LinkNameId` is available.

## Why This Idea Exists

Idea 161 closed HIR template binding identity. The next boundary is the final
symbol route after HIR has materialized names. The codebase already has
`TextTable`, `LinkNameTable`, and `LinkNameId` in several layers, but rendered
strings still remain common authority or fallback entry points:

- HIR `HirTemplateInstantiation::mangled_name` and
  `HirTemplateInstantiation::mangled_link_name_id` coexist, but many call
  sites still pass the rendered `mangled_name`.
- HIR `DeclRef` creation and resolution preserve `ref.name` and use
  `link_name_id` only when it was found from rendered spelling.
- LIR carries `LinkNameId` on `LirFunction`, `LirGlobal`, `LirExternDecl`,
  direct calls, and initializer function references, but raw `std::string`
  fields such as `name`, `callee_name`, `init_text`, and `mangled_name` remain
  reachable compatibility paths.
- BIR carries `LinkNameId` for functions, globals, calls, global loads/stores,
  memory addresses, and initializer symbol names, but raw string fields still
  participate in validation, printing, and unresolved compatibility lowering.
- Backend preparation has structured `LinkNameId` inputs, but still renders
  names early for several symbol paths.

This is the point where parser/sema/HIR identity cleanup can be lost again if
the final backend route falls back to raw strings as the normal symbol key.

## Responsibility Split

HIR owns symbol materialization:

- creates canonical final function/global/template instance names
- interns final link-visible spellings in `Module::link_names`
- preserves rendered strings for dumps and target output
- passes `LinkNameId` through `DeclRef`, template instantiation metadata, and
  global/function references

LIR owns structured low-level symbol transport:

- direct calls, globals, extern declarations, and initializer references should
  carry `LinkNameId` when known
- raw names are final spelling and compatibility payloads
- extern declaration dedup should prefer `LinkNameId`

BIR/backend own ABI and target preparation:

- validate known calls/globals by `LinkNameId`
- keep local SSA/slot/register names as route-local strings
- print final assembly/IR spelling by resolving `LinkNameId` through the name
  table
- treat invalid `LinkNameId` plus raw string as an explicit unresolved
  compatibility bridge

## In Scope

- Inventory link-visible symbol string paths across HIR, LIR, BIR, and
  backend preparation, classifying each as semantic authority, output spelling,
  route-local name, diagnostic/dump text, or compatibility bridge.
- Ensure HIR function/global/template instance construction interns final
  symbol names into `LinkNameTable` and preserves `LinkNameId` alongside any
  rendered `name`/`mangled_name`.
- Convert HIR call/global lookup handoff paths to prefer existing
  `LinkNameId` over rendered `std::string` names when complete metadata is
  present.
- Ensure HIR→LIR lowering propagates `LinkNameId` for direct calls, function
  definitions/declarations, globals, extern declarations, and initializer
  symbol references.
- Ensure LIR→BIR lowering propagates `LinkNameId` for direct calls,
  load/store global, memory-address global bases, function/global definitions,
  and initializer symbol references.
- Make BIR validation and backend preparation fail closed for covered known
  symbols when a complete `LinkNameId` miss occurs, instead of reopening raw
  string lookup.
- Keep local SSA value names, block labels, stack slots, registers, string pool
  labels, inline asm text, and final printer output as strings where they are
  route-local or textual output rather than link-visible semantic identity.
- Add focused tests where two rendered-compatible or same-spelled symbols must
  not collide once link names are interned, and where direct calls/globals keep
  their `LinkNameId` through HIR→LIR→BIR.
- Document retained raw-name bridges with owner, limitation, and removal
  condition.

## Out Of Scope

- Reworking HIR template binding keys; that was handled by idea 161.
- Rewriting ABI mangling algorithms or changing the spelling of emitted
  symbols unless required to intern an already-produced spelling.
- Replacing route-local SSA names, local slots, block labels, register names,
  or string-pool names with `LinkNameId`.
- Removing all raw strings from LIR/BIR printers or diagnostics.
- Full backend object emission redesign.
- Weakening tests, changing only output text expectations, or adding named
  symbol shortcuts.

## Candidate Anchors

- `HirTemplateInstantiation::mangled_name` /
  `mangled_link_name_id`, `Function::link_name_id`,
  `GlobalVar::link_name_id`, `DeclRef::link_name_id`, and `Module::link_names`
  in `src/frontend/hir/hir_ir.hpp`.
- HIR construction and call/global lookup helpers in
  `src/frontend/hir/hir_types.cpp`, especially
  `make_function_lookup_decl_ref`, `make_global_lookup_decl_ref`,
  template call resolution, struct method link-name lookup, and global
  declaration lowering.
- LIR symbol carriers in `src/codegen/lir/ir.hpp`:
  `LirFunction::link_name_id`, `LirGlobal::link_name_id`,
  `LirExternDecl::link_name_id`, `LirCall::direct_callee_link_name_id`,
  `initializer_function_link_name_ids`, and `LirSpecEntry`.
- HIR→LIR lowering under `src/codegen/lir/hir_to_lir/`, especially call target
  selection, global emission, extern declaration recording, and constant
  initializer emission.
- BIR symbol carriers in `src/backend/bir/bir.hpp`:
  `Function::link_name_id`, `Global::link_name_id`,
  `CallInst::callee_link_name_id`, `LoadGlobalInst::global_name_id`,
  `StoreGlobalInst::global_name_id`, `MemoryAddress::base_link_name_id`, and
  `Global::initializer_symbol_name_id`.
- LIR→BIR lowering under `src/backend/bir/lir_to_bir/`, BIR validation in
  `src/backend/bir/bir_validate.cpp`, BIR printing in
  `src/backend/bir/bir_printer.cpp`, and backend preparation under
  `src/backend/prealloc/`.

## Acceptance Criteria

- Link-visible raw string symbol paths are inventoried and classified across
  HIR, LIR, BIR, and backend preparation.
- Covered final function/global/direct-call/initializer symbol paths carry and
  consume `LinkNameId` when complete metadata is present.
- LIR extern declaration dedup and BIR call/global validation prefer
  `LinkNameId` over raw spelling.
- A complete `LinkNameId` miss for a covered known symbol fails closed or is
  reported as a structured mismatch rather than silently falling back to raw
  string lookup.
- Raw `std::string` names remain only as output spelling, diagnostics,
  route-local names, or explicitly documented compatibility bridges.
- Focused tests prove symbol identity survives HIR→LIR→BIR and that raw-name
  compatibility cannot override a valid `LinkNameId`.

## Reviewer Reject Signals

- A slice claims backend symbol authority is `LinkNameId` while direct calls,
  globals, or extern declarations still dedup/look up by raw string first for
  covered metadata.
- `mangled_name`, LLVM text, or printer output becomes the new semantic key.
- Route-local names such as SSA temporaries, stack slots, labels, registers, or
  string-pool labels are incorrectly forced into `LinkNameId`.
- A valid `LinkNameId` miss reopens raw string lookup without being marked as a
  compatibility/no-metadata path.
- Tests only update printed text and do not prove structured symbol transport.
