Status: Active
Source Idea Path: ideas/open/118_lir_bir_legacy_type_text_removal.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Remaining Legacy Type Text Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by classifying the current legacy type-text
authority surfaces and selecting the first directly provable removal family.

Legacy-surface inventory:

- `LirModule::type_decls` is still lowered into `TypeDeclMap` in
  `src/backend/bir/lir_to_bir/module.cpp`; it is active layout/lowering
  authority for all `compute_aggregate_type_layout` fallbacks, and also
  proof-only shadow for `BackendStructuredLayoutTable` parity notes.
- `TypeDeclMap` is active layout/lowering authority in aggregate layout,
  global initializer/address parsing, local/global addressing, call ABI,
  byval/sret lowering, variadic aggregate lowering, and memory intrinsic
  aggregate paths. Structured layout is used first only for parity-matching
  named structured declarations; unresolved or non-parity paths still fall
  back to legacy declaration-body parsing.
- Raw `LirTypeRef` text remains blocked type-ref authority where backend
  lowering parses `call.return_type.str()`, `arg_type_refs`/typed call text,
  GEP element types, load/store/cast/phi/select type strings, and aggregate
  field text. It is also final spelling/output data for LIR printing and
  final LLVM emission, so broad removal is out of scope for this runbook.
- Function `signature_text` remains active lowering authority for function
  return parsing, parameter parsing, variadic detection, and byval aggregate
  parameter recovery. It also remains final spelling/output data for the LIR
  printer.
- Extern `return_type_str` remains active lowering authority for extern
  declaration return ABI, with structured `return_type` only used as fallback
  input when the string path fails; it is also final spelling/output data for
  LIR extern declaration emission.
- Global `llvm_type` strings remain active layout/lowering authority for
  scalar, integer-array, aggregate global storage, initializer parsing, and
  global-address provenance. They are also final spelling/output data for LIR
  global emission.
- BIR printer call return spelling is structured-first for covered aggregate
  call returns through `CallInst::structured_return_type_name` plus
  `Module::structured_types`, matching idea 117. `CallInst::return_type_name`
  is now deferred compatibility/fallback dump-render data for those covered
  aggregate returns, but still active dump-render authority for scalar,
  pointer, inline-asm, prepared-BIR/test-constructed calls, and any call with
  no structured type context.
- Inline asm `return_type_name`, `asm_text`, `constraints`, and `args_text`
  are final spelling/output data or deferred compatibility, not a safe first
  removal target.

First directly provable removal target: remove the BIR printer fallback use of
`CallInst::return_type_name` for covered structured aggregate sret call returns
only. The replacement authority is `structured_return_type_name` plus
`Module::structured_types`, already guarded by
`backend_prepare_structured_context`.

## Suggested Next

Delegate a code-changing packet for `plan.md` Step 3 that makes
`bir_printer.cpp::render_call_type_name` stop consulting
`CallInst::return_type_name` when a valid structured aggregate return name is
present. Keep scalar/pointer/manual prepared-BIR fallbacks intact. Suggested
focused proof:
`cmake --build build-backend --target backend_prepare_structured_context_test && ctest --test-dir build-backend -j --output-on-failure -R '^backend_prepare_structured_context$'`.

## Watchouts

- Do not start with `TypeDeclMap` removal: many paths still use legacy body
  parsing as active layout/lowering authority when structured layout is absent
  or not parity-matching.
- Do not remove `return_type_name` wholesale: scalar, pointer, inline-asm, and
  manually constructed prepared-BIR tests still use it as dump/printer
  compatibility data.
- Current plan file paths name older locations; current code is under
  `src/codegen/lir/` and `src/backend/bir/lir_to_bir/`.

## Proof

Ran `git diff --check -- todo.md`; passed. This packet was inventory-only, and
the delegated ownership explicitly forbade writing `test_after.log`.
