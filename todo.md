Status: Active
Source Idea Path: ideas/open/117_bir_printer_structured_render_context.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Minimal Structured Render Context

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding a minimal BIR-facing structured type
spelling context and populating it from existing LIR structured declaration
metadata.

Concrete slice:
- Added `bir::StructuredTypeSpellingContext` on `bir::Module`, with declaration
  names, field type spellings, packed/opaque flags, and a narrow
  `find_struct_decl()` lookup helper.
- Added LIR-to-BIR population from `LirStructDecl` plus `StructNameTable` in
  the existing module lowering path, next to the backend structured layout
  table construction.
- Left `bir_printer.cpp` behavior unchanged; `--dump-bir` remains byte-stable
  for the covered subset, and `CallInst::return_type_name` remains the Step 3
  compatibility fallback.
- Added `backend_prepare_structured_context` to prove BIR context population
  while asserting the printer does not render the new structured declarations
  yet.

## Suggested Next

Delegate Step 3 to route the first covered call return type rendering through
the structured context, keeping `CallInst::return_type_name` as the fallback
for uncovered/runtime/inline-asm cases.

## Watchouts

- The new BIR context is intentionally data-only; `bir_printer.cpp` still has
  no module/context parameter threaded into instruction rendering.
- `CallInst::return_type_name` is untouched and should stay the compatibility
  fallback in Step 3.
- Inline asm raw return/argument text remains a later fallback family unless
  the next slice explicitly chooses to cover it.

## Proof

`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^(backend_cli_dump_bir_|backend_cli_dump_prepared_bir_|backend_codegen_route_x86_64_(aggregate_param_return_pair|local_aggregate_member_offsets)_observe_semantic_bir|backend_prepare_)'; } > test_after.log 2>&1`
passed: 28/28 tests. Proof log: `test_after.log`.
