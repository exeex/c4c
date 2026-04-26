Status: Active
Source Idea Path: ideas/open/117_bir_printer_structured_render_context.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Call Return Type Rendering Structured-First

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by making BIR call return type rendering prefer
structured return spelling metadata when the module structured context
recognizes the source aggregate type, while preserving existing dump text.

Concrete slice:
- Added `bir::CallInst::structured_return_type_name` as a structured metadata
  source beside the existing `return_type_name` fallback.
- Populated the new call metadata from LIR `LirTypeRef` struct identity during
  regular call lowering without removing or weakening `return_type_name`.
- Threaded `Module::structured_types` into BIR function/instruction rendering
  and made call return printing use the structured context first for covered
  sret aggregate calls, still rendering byte-stable ABI `void`.
- Extended `backend_prepare_structured_context` with a focused printer check
  proving structured-first rendering ignores a deliberately poisoned legacy
  return string for a direct aggregate-return call.

## Suggested Next

Delegate Step 4 to broaden aggregate-sensitive printer coverage now that the
first call return rendering path is structured-first and byte-stable.

## Watchouts

- `CallInst::return_type_name` remains the compatibility fallback for calls
  without recognized structured return metadata, including handwritten BIR,
  runtime intrinsics, and inline asm paths.
- The structured-first call return path currently covers source aggregate
  returns that lower to ABI sret `void`; it intentionally preserves existing
  dump text rather than printing the source aggregate spelling.
- Remaining argument type spellings, runtime intrinsic call spellings, and
  inline asm raw type text are still fallback surfaces for later inventory.

## Proof

`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^(backend_cli_dump_bir_(layout_sensitive_aggregate|is_semantic|00204_stdarg_semantic_handoff|focus_function_filters_00204)|backend_cli_dump_prepared_bir_(is_prepared|00204_stdarg_prepared_handoff)|backend_codegen_route_x86_64_(aggregate_param_return_pair|aggregate_return_pair|variadic_pair_second)_observe_semantic_bir|backend_prepare_structured_context)$'; } > test_after.log 2>&1`
passed: build plus 10/10 selected tests. Proof log: `test_after.log`.
