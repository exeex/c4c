# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Bind probes to owned carrier contracts

## Just Finished

- Plan Step 3 now binds four one-contract focused probes: new minimal store and
  array-address cases plus existing `global_load.c` and historically named but
  target-neutral `aarch64_return_zero_smoke.c`.
- Their exact first facts are store, load, GEP-with-two-zero-indices and no
  preceding cast, and `ret i32 0`; the original larger four cases remain
  integration evidence. The authority matrix records each baseline and
  focused-versus-integration contrast without claiming BIR support.

## Suggested Next

- Execute Plan Step 4 by binding each focused probe to one existing typed
  carrier convention, one producer-population rule, reachable verifier
  obligations, and exact malformed/missing/conflicting rejection coverage.

## Watchouts

- The focused array probe uses scalar ternary control so array decay emits GEP
  first. A pointer-returning helper instead stops at the unrelated closed
  pointer-signature gate, while direct indexed address syntax inserts a sext;
  neither should replace the checked focused shape.
- The comparison after the focused GEP is explicitly non-primary. Preserve
  `defined_global_array.c` as cast-first/later-GEP integration evidence and do
  not infer coverage for remaining `C-gap—Step 3 candidate` rows.

## Proof

- Fresh `cmake --build build -j2 --target c4cll` passed. For all four focused
  probes, `./build/c4cll --dump-bir --target x86_64-linux-gnu <case>` reached
  the required current family: store/load/GEP are
  `UnsupportedOrdinaryInstruction`; scalar return is `InvalidVoidReturn`.
- `--codegen llvm` confirmed the exact first body facts and no preceding cast
  for the GEP probe. `git diff --check` passed; no root proof log was written.
