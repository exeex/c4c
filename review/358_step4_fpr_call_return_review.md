# Review: idea 358 Step 4 FPR call/return route after bd0cfd1c

Active source idea path: `ideas/open/358_rv64_object_route_abi_width_edges.md`

Chosen base commit: `bd0cfd1c Record RV64 FPR route review`

Base selection rationale: the delegated packet explicitly names `bd0cfd1c` as
the prior review checkpoint for this same active idea 358 Step 4 FPR route. It
is a review artifact commit, not an activation commit, but it is unambiguous
history for the requested incremental review window. The original active idea
activation remains `8fe84dea6 [plan] Activate RV64 ABI width edge coverage
plan`; this report reviews only the requested post-checkpoint delta through
`HEAD`.

Commit count since base: 7

Working tree note: `todo.md` is dirty with a transient non-canonical review
prompt at line 6. I treated the committed `todo.md` content plus the line-6
prompt as execution context only and did not edit lifecycle state.

## Findings

No blocking findings.

Severity: low - validation proof is still narrower than the next route decision.

- `todo.md:30` correctly says to rerun `src/20030125-1.c` after the prepared
  FPR return ABI move, but the working tree has no readable `test_after.log`.
  The current packet records focused backend proof, and the focused tests cover
  the new object-emission shapes, but acceptance of the representative route
  still needs the requested rerun/classification of `src/20030125-1.c`.

Severity: info - direct-call FPR lowering remains metadata-backed.

- `src/backend/mir/riscv/codegen/object_emission.cpp:1524` only enters the FPR
  argument path when the prepared call plan says the destination bank is FPR.
  It then requires FPR value bank, register source encoding, a source value id,
  FPR source register bank, and an FPR ABI placement before emitting a move.
- `src/backend/mir/riscv/codegen/object_emission.cpp:1535` resolves the source
  through `PreparedValueHome` plus `PreparedTargetRegisterIdentity`, while
  `src/backend/mir/riscv/codegen/object_emission.cpp:1539` resolves the ABI
  destination through `target_register_identity_for_abi_register_placement`.
- `src/backend/mir/riscv/codegen/object_emission.cpp:1626` applies the same
  fail-closed pattern for FPR call results: source placement must be FPR ABI
  placement, destination must be a prepared FPR value home, and the BIR result
  type drives `fmv.s`/`fmv.d` selection.

Severity: info - before-return FPR move handling is narrowly keyed.

- `src/backend/mir/riscv/codegen/object_emission.cpp:966` rejects unsupported
  move-bundle shapes before emission: non-move ops, non-register destinations,
  cycle-temp sources, multi-register destinations, and stack destinations fail
  closed.
- `src/backend/mir/riscv/codegen/object_emission.cpp:995` admits FPR return ABI
  destinations only through prepared ABI placement, not raw `fa0` spelling.
- `src/backend/mir/riscv/codegen/object_emission.cpp:1032` emits an FPR move
  only when the source resolves through prepared FPR identity and
  `prepared_bir_value_type_for_name` finds an F32/F64 BIR value type.
- `src/backend/mir/riscv/codegen/object_emission.cpp:1706` skips the fallback
  GPR `a0` return move only for F32/F64 named return values that have a
  prepared before-return ABI move for the same block, same source value id, and
  FPR destination bank.

Severity: info - producer-side FPR identity publication is still a general ABI
mechanism, not a testcase shortcut.

- `src/target_profile.cpp:44` classifies the actual `riscv64-linux-gnu` target
  triple as `RiscvLp64D`, enabling the hard-float ABI expected for Linux GNU
  RV64 object-route representative runs.
- `src/backend/prealloc/regalloc/value_homes.cpp:363` publishes formal ABI
  register identity via prepared ABI placement, and
  `src/backend/prealloc/regalloc/value_homes.cpp:457` publishes identities for
  assigned FPR registers through the register allocator's placement metadata.
- `src/backend/prealloc/regalloc/value_homes.cpp:232` maps only the currently
  modeled RV64 FPR assigned-register pools to physical identities. Unknown
  pools/slots return no identity, so object emission rejects rather than
  parsing FPR register spellings.

Severity: info - focused tests are synthetic metadata-contract tests, not
`src/20030125-1.c` shape matches.

- `tests/backend/mir/backend_riscv_object_emission_test.cpp:3583` proves FPR
  direct-call argument/result moves through prepared call plans and ABI
  placements.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp:3761` still rejects
  a raw FPR formal home with no target identity.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp:4903` and
  `tests/backend/mir/backend_riscv_object_emission_test.cpp:4927` prove F32/F64
  before-return FPR ABI moves, while
  `tests/backend/mir/backend_riscv_object_emission_test.cpp:4951` keeps cycle
  temp, multi-width, stack-destination, and F128 move-bundle shapes rejected.

## Alignment Evidence

The active idea allows targeted RV64 FPR/simple-call ABI lowering only where
prepared BIR already models the required call/value semantics, and rejects
guessed FPR/simple-call behavior without prepared ABI metadata. The post-
`bd0cfd1c` diff stays within that boundary:

- FPR ABI identity is produced by target profile/placement metadata.
- FPR casts consume prepared FPR homes and exact BIR F32/F64 cast opcodes.
- FPR direct-call edges consume prepared call plans, ABI placements, and value
  homes.
- Before-return moves consume prepared move bundles and BIR value type metadata.

I found no source-case name matching, source-pattern matching, `print_llvm()`
substring probing, expectation downgrades for `src/20030125-1.c`, or raw FPR
`register_name` fallback parsing in the reviewed diff.

## Judgments

Idea-alignment judgment: matches source idea

Runbook-transcription judgment: plan matches idea

Route-alignment judgment: on track

Technical-debt judgment: watch

Validation sufficiency: needs broader proof

Reviewer recommendation: continue current route

## Recommendation

Continue by rerunning and classifying `src/20030125-1.c` through the RV64
backend progress proof. That is the correct next step because the latest
commits repaired the prepared FPR call/return move surfaces in a metadata-
backed, fail-closed way, and the representative must now reveal whether the
next boundary is still in-scope object emission or belongs to another idea.

Do not rewrite `plan.md` or the source idea before that rerun. If the
representative advances to a new failure, record the classification in
`todo.md` first unless it proves a true route change.
