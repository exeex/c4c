Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and Classify Link-Visible Symbol Paths

# Current Packet

## Just Finished

Completed Step 1 inventory for link-visible raw string symbol paths across HIR,
LIR, BIR, and backend preparation. Classification anchors inspected:

- HIR semantic authority: `Function::link_name_id`, `GlobalVar::link_name_id`,
  `DeclRef::link_name_id`, `HirTemplateInstantiation::mangled_link_name_id`,
  and `Module::link_names` in `src/frontend/hir/hir_ir.hpp`.
- HIR compatibility bridges: `make_function_lookup_decl_ref` and
  `make_global_lookup_decl_ref` in `src/frontend/hir/hir_types.cpp` still build
  `DeclRef` from rendered `std::string` names and recover `link_name_id` with
  `module.link_names.find(ref.name)`; template call inference still passes
  `HirTemplateInstantiation::mangled_name` through that bridge.
- HIR route-local/output names: global/function `name`, `name_text_id`, and
  rendered template `mangled_name` remain output spelling or compatibility
  payloads when paired with `LinkNameId`.
- LIR semantic authority: `LirFunction::link_name_id`,
  `LirGlobal::link_name_id`, `LirExternDecl::link_name_id`,
  `LirCallOp::direct_callee_link_name_id`,
  `LirGlobal::initializer_function_link_name_ids`, and
  `LirSpecEntry::mangled_link_name_id` in `src/codegen/lir/ir.hpp`.
- LIR output/compatibility payloads: `LirCallOp::callee`,
  `LirExternDecl::name`, `LirFunction::name`, `LirGlobal::name`,
  `LirGlobal::init_text`, `LirFunction::signature_text`, and
  `LirSpecEntry::mangled_name`. `record_extern_decl` already dedups by
  `LinkNameId` first and only uses raw name dedup when the id is invalid.
- HIR to LIR call lowering: `src/codegen/lir/hir_to_lir/call/target.cpp`
  resolves local direct calls by `LinkNameId` first and returns `nullptr` on a
  valid-id miss before trying fallback name lookup; unresolved externs record
  both spelling and id.
- HIR to LIR initializer lowering: `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`
  resolves globals/functions by `LinkNameId` where available, but `resolve_decl_name`
  still returns the raw spelling if neither structured global nor function lookup
  succeeds; this is a compatibility bridge for legacy initializer producers.
- BIR semantic authority: `Function::link_name_id`, `Global::link_name_id`,
  `CallInst::callee_link_name_id`, `LoadGlobalInst::global_name_id`,
  `StoreGlobalInst::global_name_id`, `MemoryAddress::base_link_name_id`, and
  `Global::initializer_symbol_name_id` in `src/backend/bir/bir.hpp`.
- BIR output/route-local/diagnostic text: `Value::name`, `Block::label`,
  local slot names, `CallInst::callee`, `LoadGlobalInst::global_name`,
  `StoreGlobalInst::global_name`, `MemoryAddress::base_name`, inline asm text,
  printer output, and diagnostics remain textual. `bir_printer.cpp` explicitly
  treats call target text as dump/final spelling while direct-call identity is
  `callee_link_name_id`.
- BIR validation authority: `src/backend/bir/bir_validate.cpp` validates known
  calls/globals/initializer symbols by `LinkNameId` first, checks id/name
  mismatches, and falls back to raw name lookup only when the id is invalid.
- LIR to BIR compatibility bridges: `src/backend/bir/lir_to_bir/globals.cpp`
  still keeps `is_known_raw_function_symbol` for initializer/address handling;
  `src/backend/bir/lir_to_bir/memory/local_slots.cpp` and
  `memory/provenance.cpp` still recognize `@raw_function` strings for pointer
  slot/address provenance where structured symbol metadata has not reached the
  route.
- Backend preparation compatibility bridges:
  `src/backend/prealloc/stack_layout/coordinator.cpp` interns fallback global
  spellings and `MemoryAddress::base_name` into prepared link-name tables for
  direct symbol-backed accesses; `src/backend/prealloc/prealloc.hpp` also
  re-interns BIR global spelling for computed-value/global-load planning and
  resolves same-module globals by prepared raw spelling.

First narrow implementation target: tighten the HIR template-function handoff
in `src/frontend/hir/hir_types.cpp` by adding/passing a known `LinkNameId` into
`make_function_lookup_decl_ref` for `HirTemplateInstantiation` paths that
currently pass only `mangled_name`. This is narrow, uses existing
`mangled_link_name_id`, and prevents a covered template call from recovering
semantic identity by raw rendered spelling.

## Suggested Next

Implement the HIR template-function handoff target: update
`make_function_lookup_decl_ref` or its call sites so `deduced_template_calls_`
and template call result inference preserve `HirTemplateInstantiation::mangled_link_name_id`
directly, with rendered `mangled_name` retained as output/fallback spelling.

## Watchouts

- Retained compatibility bridge: HIR `make_function_lookup_decl_ref` /
  `make_global_lookup_decl_ref`; owner HIR lowering; limitation recovers
  `LinkNameId` from rendered spelling; removal condition is all covered callers
  pass known `LinkNameId` or structured `DeclRef` metadata directly.
- Retained compatibility bridge: HIR struct method rendered-key fallback in
  `find_struct_method_link_name_id`; owner HIR method lookup; limitation uses
  `tag::method[_const]` rendered key when owner metadata is incomplete; removal
  condition is complete owner-key coverage for inherited/local method lookup.
- Retained compatibility bridge: LIR `init_text` scanning and
  `resolve_decl_name` raw fallback; owner HIR to LIR constant initializer
  emission; limitation raw LLVM initializer payload can still introduce symbol
  references without structured producer metadata; removal condition is all
  initializer symbol producers emit `LinkNameId` references.
- Retained compatibility bridge: LIR extern raw-name map in `record_extern_decl`;
  owner LIR module; limitation dedups by raw name only when `LinkNameId` is
  invalid; removal condition is all extern call declaration producers carry a
  valid id.
- Retained compatibility bridge: BIR `is_known_raw_function_symbol` and related
  pointer provenance uses in `globals.cpp`, `memory/local_slots.cpp`, and
  `memory/provenance.cpp`; owner LIR to BIR lowering; limitation recognizes
  raw `@function` text for pointer/address facts; removal condition is function
  pointer/address provenance carries `LinkNameId`.
- Retained compatibility bridge: prealloc direct symbol-backed accesses and
  computed-value/global-load planning re-intern `global_name` or
  `MemoryAddress::base_name`; owner backend preparation; limitation prepared
  tables preserve spellings after BIR instead of consuming original BIR ids;
  removal condition is prepared address/computed-value records store and resolve
  BIR `LinkNameId` directly.
- Do not treat printer output, diagnostics, SSA names, block labels, stack
  slots, registers, string-pool labels, or inline asm text as `LinkNameId`
  candidates.
- Do not claim progress through output-only expectation rewrites or unsupported
  downgrades.
- A valid `LinkNameId` miss for a covered known symbol must not reopen raw
  string lookup silently.

## Proof

Inventory-only lifecycle packet; no build/test required and no `test_after.log`
was produced. Evidence was gathered with `rg` plus focused `sed` inspection of
the plan-listed HIR, LIR, BIR, LIR-to-BIR, and backend-prealloc anchors.
