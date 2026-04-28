# Current Packet

Status: Active
Source Idea Path: ideas/open/122_bir_string_legacy_path_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory BIR String Authority

## Just Finished

Completed Step 1 inventory for idea 122. Main BIR string uses are classified
below; no implementation or test files were changed.

Semantic authority:
- `bir::Value::name` and all value maps keyed by SSA spelling still define
  ordinary value identity inside BIR blocks.
- `bir::Function::name`, `bir::Global::name`, `bir::CallInst::callee`,
  `LoadGlobalInst::global_name`, `StoreGlobalInst::global_name`, and global
  initializer symbol names are currently the BIR-level symbol identity surface.
- `bir::LocalSlot::name`, `Param::name`, load/store local `slot_name`, and
  aggregate/local-memory slot maps are BIR local storage identity.
- `bir::Block::label`, phi incoming `label`, and branch target labels are
  still retained as string fields, but ordinary lowered blocks also carry
  `BlockLabelId` through `module.names.block_labels`; the string is now mostly
  compatibility/display once `intern_known_block_labels` has run.
- `StructuredTypeDeclSpelling::name`, field `type_name`,
  `CallInst::structured_return_type_name`, `return_type_name`, and
  LIR-to-BIR `TypeDeclMap`/layout keys still carry type/layout identity by raw
  spelling in BIR or at the lowering boundary.

Display spelling:
- BIR printer output uses names, labels, type spellings, string constant names,
  and inline asm text for stable dumps and diagnostics.
- `StringConstant::bytes`, inline asm `asm_text`/`constraints`/`args_text`,
  and escaped printer text are payload spelling, not semantic identity.
- `target_triple`, `data_layout`, lowering note `phase`/`message`, and failure
  family text are report/debug payloads.

Selector/user input:
- CLI/focused dump filters use raw function/block/value selectors outside BIR
  semantics.
- LIR `LirSwitch::selector_name` and selector type text are lowering inputs
  that feed BIR values/types.

Compatibility payload:
- LIR `signature_text`, `type_decls`, `return_type_str`, `llvm_type`,
  `init_text`, `args_str`, `callee_type_suffix`, GEP indices, typed operand
  strings, and aggregate initializer text are compatibility/LLVM-text shadows
  that BIR lowering still parses.
- Tests intentionally assert some legacy mirrors remain, especially
  struct-decl/type-decl parity and extern/call type-ref compatibility.

Unresolved-id boundary:
- When `LinkNameId`, `StructNameId`, or `BlockLabelId` is invalid or cannot be
  resolved, the current path falls back to raw spelling or leaves the id
  invalid. Those are retained boundaries that should be made explicit as each
  family is cleaned.

Candidate structured-id paths:
- Existing and already-used: `BlockLabelId` plus `NameTables::block_labels` for
  block labels, branch targets, phi incoming labels, and label memory bases.
- Existing upstream, flattened at BIR boundary: `LinkNameId` for LIR
  functions, globals, extern declarations, specialization names, and direct
  callees.
- Existing upstream, partially mirrored into BIR spelling: `StructNameId` and
  `LirTypeRef` for struct type declarations, function signatures, globals,
  calls, and aggregate layout.

## Suggested Next

First implementation packet: thread `LinkNameId` through the BIR function,
global, extern declaration, and direct-call symbol boundary while preserving
the existing string spelling for printer/dump compatibility. Keep the slice to
symbol identity only: update BIR records, LIR-to-BIR lowering, validation, and
focused LIR-to-BIR tests so a drifted raw name cannot override a valid
`LinkNameId`.

## Watchouts

Keep this idea focused on BIR and the LIR-to-BIR boundary. Do not absorb MIR,
x86 renderer, parser, or HIR cleanup into this plan. Retained display,
selector, compatibility, and intentionally unresolved strings should be
classified rather than deleted.

Do not start with struct/type layout cleanup: it spans `StructNameId`,
`LirTypeRef`, legacy `type_decls`, layout parsing, and compatibility fixtures,
so it is a broader second packet after symbol identity is pinned down. Block
labels already have a structured-id path and coverage; use them as a reference
model rather than the first code-changing target.

## Proof

Inspection-only Step 1 inventory. No build or test run.

Focused proof command for the first implementation packet:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_lir_to_bir_notes|frontend_lir_(global_type_ref|extern_decl_type_ref|call_type_ref|function_signature_type_ref))$' > test_after.log 2>&1`
