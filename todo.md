Status: Active
Source Idea Path: ideas/open/118_lir_bir_legacy_type_text_removal.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Preserve Output Contracts Across Prepared BIR And Backend Runs

# Current Packet

## Just Finished

Recorded supervisor-run broader validation for `plan.md` Step 5 after the
Step 2 structured-layout-without-legacy-decl change and the Step 3 structured
sret return fallback text removal. Backend build plus the full runnable
backend test subset passed with current output contracts preserved.

## Suggested Next

Either delegate another narrow Step 2 layout-authority packet for a directly
provable remaining legacy fallback family, or move to Step 6 final
classification/handoff if no such family is selected.

## Watchouts

- The broader backend checkpoint covered runnable `backend_` tests only; 12
  disabled MIR-focus tests did not run.
- Keep separating output-contract preservation from legacy-authority removal:
  passing backend output contracts does not by itself retire the remaining
  legacy text surfaces listed below.

## Remaining Legacy Surface Inventory

- `LirModule::type_decls` and `TypeDeclMap` remain active layout/lowering
  fallback authority for unnamed aggregate text, named types without valid
  structured layout metadata, invalid structured layouts, and named structured
  types with legacy parity mismatches; they also remain proof-only shadow data
  for structured-layout parity notes.
- Raw `LirTypeRef` text remains blocked type-ref authority for call return
  parsing, typed call args, GEP/load/store/cast/phi/select type strings, and
  aggregate field text, and remains final spelling/output data for LIR printing
  and final LLVM emission.
- Function `signature_text`, extern `return_type_str`, and global `llvm_type`
  strings remain active lowering authority and final spelling/output data for
  their current declaration, ABI, global storage, initializer, and emission
  paths.
- `CallInst::return_type_name` still covers scalar/pointer returns, runtime
  intrinsics, inline asm, prepared-BIR/manual calls, calls without structured
  return context, and structured aggregate sret calls lacking
  `structured_return_type_name`.

## Proof

Supervisor-run validation checkpoint:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`;
107/107 runnable backend tests passed, and 12 disabled MIR-focus tests did not
run. Proof log: `test_after.log`.
