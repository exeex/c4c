Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Safe Structured Layout Lookups First-Class

# Current Packet

## Just Finished

Completed `plan.md` Step 2 declaration/signature ABI slice.

`BirFunctionLowerer::lower_extern_decl(...)` and
`BirFunctionLowerer::lower_decl_function(...)` now receive the module
`TypeDeclMap` and `BackendStructuredLayoutTable`. Extern declaration return
lowering, declaration-only function signature return lowering, and
declaration-only parameter ABI lowering now call the existing structured-aware
aggregate ABI helpers with module layout authority when it is available.

Legacy fallback remains available through the existing fallback helper and
through `lookup_backend_aggregate_type_layout(...)`; named `%struct` behavior
still stays parity-gated by the shared lookup rather than becoming globally
structured-first.

## Suggested Next

Continue `plan.md` Step 2 with the next safe structured layout lookup family:
pick one covered aggregate size/alignment or field-offset query family and add
focused tests around nested, packed, empty, array, or pointer-containing
aggregates as appropriate.

## Watchouts

- Keep legacy `type_decls` available as fallback and parity evidence.
- Do not remove legacy parsing helpers during this runbook.
- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.
- The module declaration call sites had to be threaded alongside the delegated
  declaration helpers so declaration lowering can actually receive module
  layout authority.

## Proof

Proof command run:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|abi_)'; } > test_after.log 2>&1`

Result: passed. Build completed and 7/7 selected tests passed.
Proof log: `test_after.log`.

Additional compile sanity check passed because the delegated `build` preset has
`ENABLE_C4C_BACKEND=OFF`: `cmake --build build-backend --target c4c_backend`.
