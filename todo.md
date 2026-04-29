Status: Active
Source Idea Path: ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit LIR, BIR, And Backend Handoff Text Authority

# Current Packet

## Just Finished

Step 4 audited LIR, BIR, and backend handoff text authority without behavior
changes.

Structured identity:

- LIR carries semantic link-visible identity through `LinkNameId` on
  functions, globals, extern decls, direct calls, specialization mangled names,
  and initializer function references. `LirModule::record_extern_decl()` dedups
  first by `extern_decl_link_name_map` and falls back to raw
  `extern_decl_name_map` only when no `LinkNameId` exists.
- LIR structured type identity exists through `StructNameId`,
  `LirStructDecl`, `LirTypeRef` structured mirrors, and
  `struct_decl_index` keyed by `StructNameId`. `LirValueId`, `LirBlockId`,
  `LirStackSlotId`, and `LirGlobalId` remain structured carriers where the
  newer LIR skeleton uses them.
- BIR imports LIR link-name tables into `bir::NameTables`, and uses
  `LinkNameId` on functions, globals, direct calls, global loads/stores,
  memory addresses, and pointer-initializer symbols when known. The BIR
  validator resolves by `LinkNameId` first and checks that any paired visible
  spelling is not inconsistent with the declared symbol.
- BIR block identity is structured after lowering interns known labels into
  `BlockLabelId`; branch terminators, conditional branches, phi incomings, and
  label memory addresses carry IDs where known. The BIR printer prefers
  structured block-label spelling and uses raw labels only for legacy/raw-only
  payloads.
- Prepared backend handoff identity is structured through `PreparedNameTables`
  and IDs: `FunctionNameId`, `BlockLabelId`, `ValueNameId`, `SlotNameId`,
  `LinkNameId`, `PreparedObjectId`, `PreparedValueId`, and
  `PreparedFrameSlotId`. Prepared addressing, liveness, regalloc, value-home,
  call-plan, storage-plan, dynamic-stack, out-of-SSA, and control-flow records
  are keyed by these IDs. x86 prepared queries resolve function spelling to
  `FunctionNameId` before consulting prepared addressing/value-location tables.
- Backend consumer code prefers structured block labels when mapping BIR blocks
  into prepared labels: `resolve_existing_consumer_block_label_id()` and
  x86 module helpers use BIR `BlockLabelId` spelling first, then raw labels
  only if no structured ID is present.

Legitimate final/display/generated text:

- LIR and BIR printers, dumps, route notes, validation errors, x86 handoff
  exceptions, debug route reports, and prepared fast-path lane/reason strings
  are display or diagnostic text.
- Final artifact text is legitimate: LLVM type spellings, SSA operand
  spellings, generated labels, string-pool names/bytes, inline asm payloads and
  constraints, target triples, data layout strings, register names, emitted asm
  labels, private data labels, and final x86/prepared assembly text.
- BIR `Value::name`, params, local slots, local load/store slot names, scalar
  temporary names, phi value names, generated switch-case labels, and
  function-local sret/copy slot names are route-local lowering handles, not
  cross-module semantic authority.
- String constants intentionally remain compatibility/final names rather than
  link-name symbols; they are byte-data globals, not semantic function/global
  identity.

Compatibility fallback:

- Raw LIR function/global/extern/callee names are retained as fallback and
  display spelling when `LinkNameId` is invalid. Backend analysis and lowering
  resolve from `LinkNameId` first via `function_name_for_reporting()`,
  `global_name_for_identity()`, and `extern_decl_name_for_identity()`.
- `FunctionSymbolSet::raw_symbol_link_name_ids`,
  `resolve_initializer_symbol_link_name_id()`,
  `resolve_known_global_address()`, and
  `resolve_pointer_initializer_offsets()` keep raw symbol fallback for legacy
  textual pointer initializers that are not normalized to `LinkNameId` at the
  parse/LIR boundary.
- `GlobalTypes` and global-address resolution are keyed by final emitted
  spelling because LIR memory/provenance conversion still bridges raw operands
  to BIR. Converted BIR globals, loads, stores, calls, and initializer symbols
  carry `LinkNameId` when available.
- Function-local maps such as `ValueMap`, `LocalSlotTypes`,
  `LocalPointerSlots`, `LocalIndirectPointerSlotSet`, `compare_exprs_`,
  `aggregate_value_aliases_`, and generated block-local alias maps are
  route-local lowering/provenance state keyed by local SSA or slot spelling.
- LIR `signature_text` parsing in BIR call ABI lowering is a compatibility
  fallback for hand-built or legacy LIR. Generated LIR carries structured
  `signature_params`, `signature_return_type_ref`, and
  `signature_param_type_refs`, and BIR prefers those when present.
- BIR block/phi/branch label strings remain compatibility fallback for raw-only
  BIR payloads; structured `BlockLabelId` is authoritative when present.
- Prepared helper overloads accepting raw function, block, or value spelling
  resolve immediately through `PreparedNameTables` and then consult ID-keyed
  prepared data. The raw spelling is ingress compatibility, not stored
  authority.

Suspicious authority:

- No new suspicious link-visible symbol, block-label, value-name, local-slot,
  diagnostic, dump, or final emitted-text authority was found in LIR, BIR, or
  the prepared/x86 handoff. These surfaces are either ID-backed, local to one
  lowering route, or final/display text.
- The remaining later-IR suspicious family is aggregate/type layout authority
  across the LIR to BIR/backend boundary: `LirModule::type_decls`,
  `TypeDeclMap`, `compute_aggregate_type_layout()`,
  `lookup_backend_aggregate_type_layout()`,
  `BackendStructuredLayoutTable`, `StructuredTypeSpellingContext`, and
  aggregate/global initializer layout parsing still accept final `%type` text
  and legacy textual type-decl bodies as layout authority. Structured
  `LirStructDecl`/`StructNameId` layouts are preferred when present and parity
  checked, but lookup is still driven by final type spelling and falls back to
  legacy text when structured layout is missing or parity cannot be used.
- A new focused follow-up idea is needed before Step 5 for the LIR/BIR/backend
  aggregate layout/type-decl text bridge. This should be separate from ideas
  132-137 because it is a later-IR/backend handoff cleanup, not Parser, Sema,
  or HIR owner/member/template lookup.

## Suggested Next

Before Step 5, have the plan owner create a focused open follow-up idea for the
LIR/BIR/backend aggregate layout and type-declaration text bridge described
above. Then Step 5 can summarize the full audit inventory and confirm every
suspicious family maps to an open follow-up.

## Watchouts

Do not treat every final type or assembly string as suspect: the follow-up
should target layout/provenance decisions that still depend on final `%type`
text or legacy `type_decls`, not printers, diagnostics, inline asm text,
register names, string bytes, or final emitted assembly. Keep the raw symbol
initializer fallback separate from this unless it proves to need implementation
work; it already resolves to `LinkNameId` when producer data is available.

## Proof

Not run per packet: no build proof required for read-only LIR/BIR/backend
text-authority audit and `todo.md` update; tests were explicitly out of scope.
