Status: Active
Source Idea Path: ideas/open/118_lir_bir_legacy_type_text_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Remove Covered BIR Printer Fallback Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 3 for the selected covered structured aggregate sret
call-return family. LIR-to-BIR lowering no longer populates
`CallInst::return_type_name` when the lowered call is returned via sret and has
`structured_return_type_name`; byte-stable printing remains routed through
`structured_return_type_name` plus `Module::structured_types`.

Strengthened `backend_prepare_structured_context` so the real lowered
aggregate call proves `return_type_name` is empty before the test injects stale
legacy sentinel text to guard the printer fallback route.

## Suggested Next

Delegate the next narrow Step 3 packet only after choosing another structured
printer-covered family from the remaining fallback inventory, or return to the
deferred Step 2 layout-removal route if the supervisor wants layout authority
progress before more printer metadata cleanup.

## Watchouts

- `CallInst::return_type_name` remains active compatibility/fallback data for
  scalar returns, pointer returns, runtime intrinsic calls, inline asm,
  prepared-BIR/manual constructed calls, and calls without structured return
  context.
- Structured aggregate sret calls without `structured_return_type_name` still
  retain legacy `"void"` fallback text.
- This packet removed fallback payload population for one covered family; it
  did not remove the `return_type_name` field or change prepared-BIR printer
  semantics.

## Remaining Legacy Surface Inventory

- `LirModule::type_decls` and `TypeDeclMap` remain active layout/lowering
  authority for many aggregate, global, call ABI, byval/sret, variadic, and
  memory-intrinsic paths; they also remain proof-only shadow data for
  structured-layout parity notes.
- Raw `LirTypeRef` text remains blocked type-ref authority for call return
  parsing, typed call args, GEP/load/store/cast/phi/select type strings, and
  aggregate field text, and remains final spelling/output data for LIR printing
  and final LLVM emission.
- Function `signature_text`, extern `return_type_str`, and global `llvm_type`
  strings remain active lowering authority and final spelling/output data for
  their current declaration, ABI, global storage, initializer, and emission
  paths.
- After this packet, `CallInst::return_type_name` still covers scalar/pointer
  returns, runtime intrinsics, inline asm, prepared-BIR/manual calls, calls
  without structured return context, and structured aggregate sret calls lacking
  `structured_return_type_name`.
- Inline asm `return_type_name`, `asm_text`, `constraints`, and `args_text`
  remain final spelling/output data or deferred compatibility.

## Proof

Ran `{ cmake --build build-backend --target backend_prepare_structured_context_test && ctest --test-dir build-backend -j --output-on-failure -R '^backend_prepare_structured_context$'; } > test_after.log 2>&1`; passed. Proof log:
`test_after.log`.
