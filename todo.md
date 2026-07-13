Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate the import spine and CFG publication

# Current Packet

## Just Finished

- Step 5 consumer-seam packet replaced the prealloc-era backend facade with an
  LIR-reference-only `BackendModuleInput` and retained only the active caller
  contracts: options, dump-stage names, object result, `emit_module`,
  `emit_module_object`, and `dump_module`.
- Every backend operation now imports through `lower_lir_to_raw_bir()` first.
  Semantic-BIR requests render deterministic function/block order, link names,
  declaration state, and terminators from read-only `RawBir` views; importer
  failures preserve structured code/function/block/detail diagnostics.
- BIR-to-MIR, PreparedBir, MIR summary/trace, and object emission now reject
  explicitly instead of falling back to LLVM text, empty success, or legacy
  target codegen.
- `c4c_backend` now compiles only `backend.cpp` plus the active new-BIR sources;
  no legacy, prealloc, MIR, or quarantined importer translation unit remains in
  its compile database entries.

## Suggested Next

- Continue Step 5 by choosing the next bounded semantic importer family needed
  before a new BIR-to-MIR seam can consume more than the bootstrap CFG slice.

## Watchouts

- The full default build now reaches an external quarantined caller:
  `src/apps/c4c-as.cpp` directly includes
  `backend/mir/riscv/codegen/object_emission.hpp`, which in turn includes the
  removed prealloc module header. That app seam is outside this packet; do not
  restore the old sources to satisfy it.
- Route-debug focus options remain only for source compatibility and have no
  effect until a new MIR route exists. Later dump-stage enum values likewise
  remain compile-time names but deliberately fail at runtime.

## Proof

- A packet-local C++17 runtime executable linked against `c4c_backend` and
  passed semantic rendering for declaration, void return, and branch;
  structured importer error propagation; explicit normal emit failure;
  unsupported PreparedBir/MIR dumps; and diagnostic-only empty object results.
- `cmake --build --preset default -j 2 --target c4c_backend` and
  `cmake --build --preset default -j 2 --target c4cll` passed.
- `cmake --build --preset default -j 2` reached the next external app seam and
  failed at the direct RISC-V MIR include in `src/apps/c4c-as.cpp`.
- `jq` inspection of compile-database `file` entries confirmed no backend
  legacy, prealloc, MIR, or quarantined importer translation units;
  `git diff --check` passed. Temporary proof source/binary were removed.
- The delegated do-not-touch set excludes logs, so this proof did not rewrite
  `test_after.log`.
