Status: Active
Source Idea Path: ideas/open/125_lir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory LIR String Lookup Surfaces

# Current Packet

## Just Finished

Completed Step 1 inventory for `src/codegen/lir` string lookup surfaces.
Major classifications:

- Pure text lookup with structured IDs already nearby:
  `dedup_globals` and `dedup_functions` in
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` already prefer
  `LinkNameId` then `TextId`, but still retain raw `std::string` fallback maps
  (`best_by_name`) when HIR lacks stable IDs.
- Semantic lookup needing conversion:
  `eliminate_dead_internals` in
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` builds
  `std::unordered_set<std::string>` and
  `std::unordered_map<std::string, size_t>` from rendered `@name` references,
  then decides which discardable functions survive by rendered function name.
  Direct calls already carry `direct_callee_link_name_id`, and
  `LirFunction` carries `link_name_id`, so this is the first bounded
  conversion target.
- Semantic lookup with legacy fallback:
  `StmtEmitter::find_local_target_function` and const-init function/global
  resolution prefer `LinkNameId` or object IDs when available, then fall back
  to `find_function_by_name_legacy`, `select_global_object(name)`, or raw
  `DeclRef::name`.
- Compatibility bridge:
  `LirModule::record_extern_decl` keeps
  `extern_decl_link_name_map` plus `extern_decl_name_map`; the name map is a
  fallback bridge for calls without `LinkNameId`, and link-name entries migrate
  existing name entries when semantic identity appears.
- Struct/type bridge:
  `verify_struct_decl_shadows`, `find_declared_struct_name_id`, and related
  type-ref mirror checks compare structured `StructNameId`/`LirTypeRef`
  records against legacy rendered `type_decls` or signature text. These are
  compatibility/shadow checks, not first conversion targets.
- Final spelling/display:
  `lir_printer.cpp`, `emitted_link_name` helpers, `llvm_global_sym`,
  `LirOperand`/`LirTypeRef` text, string-pool names, diagnostics, labels, and
  LLVM opcode/predicate text remain spelling, dump, or output surfaces unless
  used as lookup authority.
- Unresolved boundaries:
  `mod.struct_defs.find(ts.tag)`, field-name comparisons, builtin-name checks,
  special identifiers such as `__func__`, and parser-like scans of legacy LIR
  operand text depend on upstream HIR metadata or LLVM syntax payloads and
  should not be folded into the first packet.

## Suggested Next

Convert the `eliminate_dead_internals` reachability path to prefer
`LinkNameId` for LIR functions and direct call references, retaining rendered
`@name` scanning only as a legacy fallback for operands/signature text that do
not yet carry structured identity.

## Watchouts

Keep the dead-internal conversion behavior-preserving: non-discardable
functions and global initializers still seed reachability, and legacy text
scans remain for string-only references. Do not convert final LLVM spelling,
diagnostic text, or struct shadow verification into semantic authority work.

## Proof

Inventory-only `todo.md` update; no build or test proof required by the
delegated packet. Existing `test_after.log` was not modified.
