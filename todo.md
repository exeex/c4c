Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fence Or Convert Text-Pool And String-Constant Identity

# Current Packet

## Just Finished

Step 2 - Fence Or Convert Text-Pool And String-Constant Identity completed for
the LIR string-pool to BIR direct-call bridge.

`bir::StringConstant` now carries `TextId name_id` as the structured
text-pool identity. `StringConstant::name` remains compatibility/display
spelling for raw-only BIR and output, with the owner/removal condition noted in
the BIR field comment.

`src/backend/bir/lir_to_bir.cpp` now materializes BIR string constants into the
lowered module's `NameTables::texts` and rewrites direct-call string pointer
args through the carried `TextId`. The rewrite no longer owns a separate raw
name table that can become semantic authority.

`tests/backend/backend_lir_to_bir_notes_test.cpp` now checks that lowered BIR
string constants carry resolvable `TextId` identity, and adds a raw-only global
fixture proving the string-pool bridge does not recover through identical raw
spelling when no BIR string-constant `TextId` exists.

## Suggested Next

Execute Step 3 against link-visible symbol identity. Start with the smallest
metadata-rich BIR/prealloc path that can prove a stale or mismatched
`LinkNameId` does not recover through raw rendered symbol spelling.

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

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 failed out of 109 run, with 12 disabled
backend CLI trace/focus tests not run.
