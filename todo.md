Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Link-Visible Symbol Identity

# Current Packet

## Just Finished

Step 3 - Convert Link-Visible Symbol Identity completed a narrow LIR-to-BIR
function/extern identity fence.

`src/backend/bir/lir_to_bir/module.cpp` now treats non-invalid
`LirExternDecl::link_name_id` and `LirFunction::link_name_id` values as
semantic authority while building the LIR-to-BIR function-symbol table. If a
present id cannot resolve through the module link-name table, module lowering
fails closed before raw-name compatibility indices are populated. Raw function
and extern spelling remains compatibility identity only for no-id payloads.

`tests/backend/backend_lir_to_bir_notes_test.cpp` now includes a focused stale
extern `LinkNameId` fixture proving an unresolved metadata-rich extern/direct
call path does not lower through its raw rendered spelling, and records the
fail-closed module note.

## Suggested Next

Continue Step 3 by reviewing the remaining direct-call and pointer-to-symbol
bridges that still resolve via raw symbol spelling for no-id compatibility.
Keep the next packet to one bridge and prove that present `LinkNameId` metadata
cannot fall back through the raw compatibility table on a miss.

## Watchouts

- Do not start AArch64 backend implementation work.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat printer, assembler, linker, diagnostic, debug-focus, or
  route-local spelling as semantic identity.
- If parser, Sema, HIR, or LIR source intent blocks this work, record a
  separate open idea instead of expanding this plan.
- Step 2 intentionally did not add an upstream `TextId` field to
  `LirStringConst`; the structured carrier is created at the LIR-to-BIR
  boundary in the BIR module text table. If a later slice needs producer-owned
  text-pool ids, record that as a separate upstream carrier initiative instead
  of widening this plan.
- x86 `module/data.cpp` raw same-module global lookup is currently final
  assembler/output spelling territory. Do not convert that before the upstream
  BIR/prepared link-name carriers are fenced.
- This slice fenced the LIR-to-BIR function-symbol table at the module
  boundary, but `calling.cpp` still has local display/identity helpers that
  return raw spelling. They are currently guarded by the module-boundary check;
  review them before claiming Step 3 broadly complete.

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 failed out of 109 run, with 12 disabled
backend CLI trace/focus tests not run.
