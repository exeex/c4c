# Current Packet

Status: Active
Source Idea Path: ideas/open/780_lir_cross_function_value_id_ownership_restoration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair the bounded shared ownership model

## Just Finished

- Step 1 — Diagnose cross-function native-ID ownership: `FnCtx` points to the
  locally emitted `LirFunction`; `fresh_value` calls that shell's
  `alloc_value()`, whose `next_value_id` starts at zero for every function.
  The module lowers each shell independently and then moves it into
  `module.functions`, so two functions legitimately emit the same numeric
  `LirValueId` (for example ID 0).
- `verify_module` nevertheless builds `instruction_result_owners` as a
  module-wide map keyed solely by numeric `LirValueId`. During
  `verify_function_value_ownership`, a flagged
  `LirCastOp::requires_native_result_authority` result is rejected whenever
  that numeric ID is also recorded for another `LirFunction`. Thus a valid
  current-function cast allocation is falsely classified as foreign solely
  because a separate function has an ordinary result with the same
  per-function ID.
- Focused reproduction: `ctest --test-dir build --output-on-failure -R
  '^llvm_gcc_c_torture_src_pr52129_c$'` failed as expected with
  `LirCastOp.result: standalone native cast result LirValueId is owned by
  another LirFunction`. `pr52129.c` defines `foo`, `bar`, and `main`, making
  it a credible existing multi-function proof case.
- Bounded repair seam: reconcile the native value-ID namespace used by
  `LirFunction::alloc_value`/`StmtEmitter::fresh_value` with the verifier's
  module-wide numeric owner map, so native IDs are unique across functions
  (or equivalently make all ownership lookup unambiguously function-scoped
  without weakening current-function checks). The next packet must choose and
  apply one shared-model representation consistently; it must not touch
  PHI/generic producers.

## Suggested Next

- Step 2: implement the smallest consistent shared native-ID allocation and
  ownership repair at the identified allocation/verifier seam, then run its
  fresh-build focused proof.

## Watchouts

- A logical-only exception is invalid: the flagged logical cast is already
  allocated by the shared `fresh_value` path and the failure arises from the
  verifier's cross-function numeric ownership model, not its cast semantics.
  Exempting it would permit the same ambiguous native ID while leaving genuine
  malformed/foreign authority indistinguishable; retain the flag-gated,
  fail-closed verifier contract. Do not absorb PHI/generic producer migration.

## Proof

- Diagnostic proof ran (no build required; current tree already has `build`):
  `ctest --test-dir build --output-on-failure -R
  '^llvm_gcc_c_torture_src_pr52129_c$'` -> expected 1/1 failure with the
  foreign-owner diagnostic. No canonical root log was written, per this
  diagnostic packet. Step 2 must use a fresh build plus a matching focused
  multi-function proof; Step 4 requires a fresh full-suite candidate.
