Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Link-Visible Symbol Identity

# Current Packet

## Just Finished

Step 3 - Convert Link-Visible Symbol Identity reviewed and fenced the
call-pointer argument bridge in
`src/backend/bir/lir_to_bir/memory/provenance.cpp`.

`lower_call_pointer_arg_value()` still accepts raw global operands such as
`@semantic_callee` and uses `FunctionSymbolSet::find_raw_symbol_link_name_id`
to keep no-id imported function pointer arguments working. That raw lookup is
now explicitly fenced as a Step 3 no-id compatibility bridge: its owner is the
call-pointer argument bridge, its limitation is that `FunctionSymbolSet` is
populated only after `LinkNameId` declarations resolve at the module boundary,
and its removal condition is LIR pointer operands carrying `LinkNameId`
metadata directly.

No focused test was added because this packet only documents/fences existing
behavior. The existing metadata-rich direct-call and extern/global
`LinkNameId` boundary tests continue to prove that missing or stale ids fail
before raw symbol lookup can recover them.

## Suggested Next

Continue Step 3 by reviewing the remaining raw pointer-to-function bridge in
`memory/provenance.cpp`, especially
`resolve_local_aggregate_pointer_value_alias()`, and either fence its no-id
compatibility lookup or document why an existing `LinkNameId` boundary already
blocks metadata-rich fallback.

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
- The direct-call bridge review is complete, and the local direct pointer-store
  bridge is now fenced. Remaining Step 3 risk is in provenance paths that
  accept raw `@symbol` operands without per-operand `LinkNameId` carriers.
- The local pointer-store fence relies on `FunctionSymbolSet` construction
  failing closed for metadata-bearing functions or extern declarations whose
  `LinkNameId` spelling cannot be resolved; do not bypass that module-boundary
  check in a later pointer bridge.
- The call-pointer argument fence has the same boundary assumption: it is only
  for no-id imported function pointer arguments, not a way to recover corrupted
  metadata-rich function identity.

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 failed out of 109 run, with 12 disabled
backend CLI trace/focus tests not run.
