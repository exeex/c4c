Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Link-Visible Symbol Identity

# Current Packet

## Just Finished

Step 3 - Convert Link-Visible Symbol Identity completed a narrow LIR-to-BIR
global identity fence.

`src/backend/bir/lir_to_bir/module.cpp` now treats a non-invalid
`LirGlobal::link_name_id` as semantic authority for lowered BIR global
identity. If the id cannot resolve through the LIR link-name table, lowering
fails closed instead of recovering through `LirGlobal::name`. Raw global names
remain compatibility identity only for no-id globals.

`tests/backend/backend_lir_to_bir_notes_test.cpp` now includes a focused stale
global `LinkNameId` fixture proving an unresolved metadata-rich global does not
lower through its raw rendered spelling, and records the fail-closed module
note.

## Suggested Next

Continue Step 3 with the next smallest link-visible identity fence. A natural
next packet is to apply the same unresolved-id fail-closed rule to
function/extern symbol identity at the LIR-to-BIR module boundary, while
preserving raw-name compatibility only for no-id functions or declarations.

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
- This slice intentionally fenced only LIR global identity. Function and extern
  `LinkNameId` spelling helpers still need separate review before claiming
  Step 3 broadly complete.

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 failed out of 109 run, with 12 disabled
backend CLI trace/focus tests not run.
