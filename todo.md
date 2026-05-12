Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Link-Visible Symbol Identity

# Current Packet

## Just Finished

Step 3 - Convert Link-Visible Symbol Identity reviewed the LIR-to-BIR
direct-call bridge and found it already fenced by the earlier function-symbol
module-boundary conversion.

`src/backend/bir/lir_to_bir/calling.cpp` marks direct calls metadata-rich when
they carry `direct_callee_link_name_id` or a structured callee signature, then
fails the direct-call semantic family unless metadata-rich calls carry a
signature and the direct callee `LinkNameId` is present in
`FunctionSymbolSet::link_name_ids`. Only after those checks does the bridge
derive the semantic callee spelling used for lowering. Because the raw
function-symbol map is not consulted on this direct-call path, a present stale
or unknown `LinkNameId` cannot recover through identical raw callee spelling.

`tests/backend/backend_lir_to_bir_notes_test.cpp` already contains focused
coverage for the reviewed bridge:
`expect_metadata_rich_direct_call_without_link_name_id_fails_closed`,
`expect_metadata_rich_direct_call_with_unknown_link_name_id_fails_closed`, and
`expect_metadata_rich_direct_call_without_signature_fails_closed`.

## Suggested Next

Continue Step 3 by reviewing one pointer-to-symbol bridge in
`memory/provenance.cpp` or `memory/local_slots.cpp` that still uses
`find_raw_symbol_link_name_id` for no-id compatibility. Keep the packet focused
on one bridge and either add a stale-id proof or record why the module-boundary
function-symbol fence already blocks metadata-rich fallback.

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
- The direct-call bridge review is now complete; remaining Step 3 risk is in
  pointer-value and provenance paths that accept raw `@symbol` operands without
  per-operand `LinkNameId` carriers.

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 failed out of 109 run, with 12 disabled
backend CLI trace/focus tests not run.
