Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Link-Visible Symbol Identity

# Current Packet

## Just Finished

Step 3 - Convert Link-Visible Symbol Identity reviewed and fenced the
link-visible local display/identity helpers in
`src/backend/bir/lir_to_bir/calling.cpp`.

The declaration identity helpers still fall back to raw function/extern
spelling for no-id LIR declarations, and direct-call lowering still falls back
to parsed raw callee spelling for no-id direct calls. Those paths are now
explicitly fenced as Step 3 compatibility bridges: their owner is the
declaration/direct-call identity bridge in `calling.cpp`, their limitation is
that module construction rejects unresolved `LinkNameId` functions/externs and
direct calls require `FunctionSymbolSet` membership before metadata-rich
lowering can use the fallback, and their removal condition is LIR declarations
and direct-call operands always carrying resolvable `LinkNameId` metadata.

No focused test was added because this packet only documents/fences existing
behavior. The existing metadata-rich direct-call and extern/global
`LinkNameId` boundary tests continue to prove that missing or stale ids fail
before raw display spelling can recover them.

## Suggested Next

Continue Step 3 by reviewing any remaining link-visible raw/no-id compatibility
bridges outside the already fenced direct-call, pointer-store, call-pointer
argument, local-aggregate pointer-value alias, and declaration identity paths.

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
- The direct-call bridge review is complete, and the local direct pointer-store,
  call-pointer argument, and local-aggregate pointer-value alias bridges are
  now fenced.
- The `calling.cpp` declaration identity and direct-call fallback review is
  complete. Those fallbacks are no-id compatibility only; do not bypass the
  module-boundary `LinkNameId` checks or `FunctionSymbolSet` membership when
  changing them later.
- The local pointer-store fence relies on `FunctionSymbolSet` construction
  failing closed for metadata-bearing functions or extern declarations whose
  `LinkNameId` spelling cannot be resolved; do not bypass that module-boundary
  check in a later pointer bridge.
- The call-pointer argument and local-aggregate pointer-value alias fences have
  the same boundary assumption: they are only for no-id imported function
  pointer operands, not a way to recover corrupted metadata-rich function
  identity.

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 failed out of 109 run, with 12 disabled
backend CLI trace/focus tests not run.
