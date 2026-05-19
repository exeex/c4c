# Backend Regex Failure Family Inventory

Status: Open
Created: 2026-05-19

## Intent

Use the main build `ctest -R backend` result as an umbrella inventory, then
split focused repair ideas only when a failure group points to a semantic
backend capability instead of one testcase shape.

## Why This Exists

The AArch64 backend c-testsuite route is now registered inside the main
`/workspaces/c4c/build` tree, so the normal backend regex baseline includes
both local backend tests and external c-testsuite backend runtime tests.

## Current Evidence

The observed backend-regex command was:

```bash
ctest -j10 -R backend
```

Run from `/workspaces/c4c/build`, this selected 352 backend-matching tests,
including 212 `c_testsuite_aarch64_backend_*` tests. The user observed about
80 failing tests. That result is not yet a classified inventory: it may mix
unit backend regressions, AArch64 c-testsuite runtime failures, frontend
handoff failures, timeout/hang cases, and already-known residual families.

The inventory should be captured from the main build tree, not from the old
side build:

```bash
cd /workspaces/c4c/build
ctest -j10 -R backend --output-on-failure
```

The currently known registration shape is:

```text
352 tests selected by ctest -R backend
212 tests are c_testsuite_aarch64_backend_*
about 80 tests were observed failing by the user
```

The AArch64 backend c-testsuite outputs now live under the main build tree:

```text
/workspaces/c4c/build/c_testsuite_aarch64_backend/
```

This matters because `ctest -j` from `/workspaces/c4c/build` now includes the
AArch64 backend c-testsuite route in the normal baseline instead of relying on
the old `/workspaces/c4c/build-aarch64-scan` side tree.

## In Scope

- Capture a fresh `ctest -j10 -R backend --output-on-failure` log from the main
  build tree.
- Classify failures by source:
  - local backend/unit/CLI tests;
  - AArch64 c-testsuite backend runtime tests;
  - frontend or prepared-module handoff failures reached through backend tests;
  - timeout/hang or runtime-output-storm cases.
- Compare the failure list against recently closed AArch64 owners 285 through
  294 before deciding whether anything needs reopening.
- Create focused `ideas/open/*.md` files for semantic repair families.
- Switch lifecycle state to a focused idea before implementation.

## Out of Scope

- Implementing fixes inside this umbrella idea.
- Treating `ctest -R backend` as one monolithic failure bucket.
- Claiming backend progress by changing expectations, allowlists, unsupported
  classifications, CTest registration, timeout policy, or runner behavior.
- Reopening closed AArch64 owner ideas from failing counts alone.
- Matching exact c-testsuite filenames, local test names, or emitted
  instruction strings instead of identifying the semantic owner.
- Asking an executor to run broad runtime tests without timeout and stale
  process cleanup.

## Completion Criteria

- `todo.md` records a current classified `ctest -R backend` failure inventory.
- Local backend regressions are separated from AArch64 external c-testsuite
  failures.
- At least one focused repair idea is created when a tractable semantic owner
  is found, or `todo.md` explains why no owner is ready to split.
- Timeout/hang cases are quarantined, deferred, or split into a safe
  hang-specific idea.
- The active lifecycle state switches away from this umbrella idea before any
  implementation work starts.
- When a focused owner is split, completed, or rejected, this source idea gets
  a durable deactivation note that records the owner decision, proof scope, and
  remaining buckets, following the style of idea 284.

## Deactivation Note

This section is intentionally present from activation. It should accumulate
durable lifecycle notes as the umbrella inventory splits focused owners, then
reactivates for later classification passes.

Initial 2026-05-19 state:

- Main build now registers the AArch64 backend c-testsuite route directly.
- `ctest -R backend` selects 352 backend-matching tests, including 212
  `c_testsuite_aarch64_backend_*` tests.
- The user observed about 80 failing tests from `ctest -j10 -R backend`.
- The first active runbook step is to capture and classify that failure set
  from `/workspaces/c4c/build`, then split focused owner ideas before coding.

Durable inventory findings to preserve:

- This umbrella exists because the baseline surface changed: AArch64 backend
  c-testsuite is no longer a side proof under `build-aarch64-scan`; it is part
  of the main build's backend regex and normal `ctest -j` baseline.
- Any future pass-count claim must state whether it is for full main-build
  baseline, `ctest -R backend`, local backend tests only, or
  `c_testsuite_aarch64_backend_*` only.
- `ctest -R backend` is an imprecise regex selector. It includes local backend
  unit/CLI tests and external AArch64 c-testsuite runtime tests, so failures
  must be classified before repair work starts.
- Recently closed AArch64 owners 285 through 296 remain valid unless a new
  generated-code or proof artifact contradicts their closure boundary.
- Timeout/hang/runtime-output-storm cases remain environment-sensitive. Broad
  runtime scans require timeout plus stale-process cleanup before their logs are
  trusted.

Deactivation 2026-05-19 inventory result:

- The captured backend regex inventory selected 352 tests: 272 passed and 80
  failed.
- All 80 current failures are `c_testsuite_aarch64_backend_*` tests.
- Failure buckets are 38 machine-printer failures, 14 `lir_to_bir` admission
  failures, 27 runtime failures, and 1 timeout.
- Closed AArch64 owners 285 through 294 remain valid by current evidence; no
  current count alone reopens them without generated-code or proof evidence
  that contradicts their closure boundaries.
- The highest-value focused split is the 22-case fused compare-branch
  machine-printer/lowering operand-form family:
  `00030`, `00034`, `00037`, `00038`, `00041`, `00054`, `00055`, `00057`,
  `00059`, `00076`, `00077`, `00085`, `00092`, `00093`, `00101`, `00127`,
  `00200`, `00203`, `00207`, `00212`, `00214`, and `00215`.
- Implementation work should move to the focused fused compare-branch owner,
  where progress means semantic operand publication/printing for
  compare-branch forms, not filename matching or expectation/runner changes.

Focused owner closure 2026-05-19:

- Focused idea 296, `ideas/closed/296_aarch64_fused_compare_branch_operand_forms.md`,
  is closed as complete for the fused compare-branch operand-form owner.
- The accepted focused proof in `test_before.log` covers the 27-test focused
  scope and reports 23 passed / 4 failed. Earlier committed focused slices
  moved the scope from 5/27 to 23/27 overall, with the final accepted slice
  moving from 21/27 to 23/27.
- The repaired owner includes immediate-left compare operands, both-immediate
  constant compares, and non-encodable register/immediate compare operands.
  `00041` and `00203` now pass.
- Residual `00200` is a runtime mismatch and residual `00207`, `00214`, and
  `00215` are scalar add/xor immediate printer limits. They are outside the
  fused compare-branch operand-form closure boundary and should be considered
  by a later inventory pass before any new focused repair owner is split.
- The next lifecycle pass should re-inventory or classify the remaining
  backend-regex buckets without reopening closed owners 285 through 296 unless
  generated-code or proof evidence contradicts their closure boundaries.

Deactivation 2026-05-19 post-296 inventory result:

- The refreshed backend-regex inventory after closed owner 296 selected 352
  tests: 290 passed and 62 failed.
- All 62 refreshed failures are `c_testsuite_aarch64_backend_*` tests.
- Failure buckets are 19 machine-printer failures, 14 `lir_to_bir` admission
  failures, 28 runtime failures, and 1 timeout.
- Closed AArch64 owners 285 through 296 remain valid by current evidence; no
  refreshed failure contradicts their closure boundaries without separate
  generated-code or proof evidence.
- The next focused split is `lir_to_bir` local-memory admission, centered on
  the 9 GEP local-memory cases `00143`, `00157`, `00176`, `00181`, `00182`,
  `00185`, `00195`, `00205`, and `00209`, with store/load boundary checks
  `00046`, `00140`, `00216`, and `00218`.
- `00204` is preserved as a separate bootstrap global aggregate/array
  semantics gate and should not be folded into the local-memory owner without
  evidence.
- Implementation work should move to the focused local-memory admission owner,
  where progress means semantic `lir_to_bir` admission for local-memory
  GEP/store/load forms, not filename matching or expectation, unsupported,
  runner, timeout, or CTest-registration changes.

Step 4 split 2026-05-19:

- Focused idea 298,
  `ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md`, is open
  as the residual semantic `lir_to_bir` global/pointer/aggregate projection
  owner.
- The split uses accepted broad backend-regex baseline `test_before.log`: 352
  selected, 291 passed, and 61 failed.
- The new owner covers global scalar-array GEPs `00176` and `00181`,
  pointer-value/parameter GEPs `00182` and `00209`, global dynamic aggregate
  member GEPs `00195` and `00205`, bootstrap/global aggregate semantics
  `00204`, and `00216` as a pointer-parameter/flexible-array aggregate
  projection boundary case.
- Machine-printer residuals, runtime nonzero/mismatch buckets, and the
  standalone `00220` timeout remain separate inventory buckets unless future
  generated-code or diagnostic evidence proves a shared semantic owner.

Lifecycle switch 2026-05-19:

- Umbrella inventory idea 295 is parked after splitting focused idea 298.
- Active implementation should move to idea 298 before code edits begin.
- Remaining inventory buckets after this switch are machine-printer residuals,
  runtime nonzero/mismatch buckets that still need generated assembly or
  narrower probes, and standalone timeout `00220`.
- Re-activate this umbrella only for a later classification pass or for
  splitting another focused owner from those remaining buckets.

Post-298 split 2026-05-19:

- Focused idea 298 is closed as the `lir_to_bir`
  global/pointer/aggregate-projection owner.
- The committed post-298 backend-regex inventory records 352 selected tests,
  60 residual failures, and no observed local backend/unit failures.
- Residual buckets are 28 c-testsuite machine-printer/frontend failures, 18
  runtime nonzero failures, 13 runtime mismatch failures, and standalone
  timeout `00220`.
- The next focused split is idea 299,
  `ideas/open/299_aarch64_scalar_immediate_materialize_or_encoding_fallback.md`,
  covering scalar add/sub/bitwise immediate materialization or encoding
  fallback for machine printing.
- The split owner is based on the largest crisp machine-printer diagnostic
  bucket: `00031`, `00104`, `00143`, `00207`, `00213`, `00214`, `00215`, and
  `00218`.
- Remaining scalar cast, mul/div/rem, call-boundary, memory store
  source/symbol, semantic `lir_to_bir`, unsigned reduction, runtime
  nonzero/mismatch, and timeout buckets remain parked under this umbrella for
  later classification or separate focused splits.
- Active implementation should move to idea 299 before code edits begin.

Focused owner closure 2026-05-19:

- Focused idea 299,
  `ideas/closed/299_aarch64_scalar_immediate_materialize_or_encoding_fallback.md`,
  is closed as complete for the scalar add/sub/bitwise immediate
  materialization or encoding-fallback owner.
- Close proof used matching backend-regex reruns in `test_before.log` and
  `test_after.log`: both selected 352 tests, stayed at 294 passed and 58
  failed, and introduced no new failures. This was non-regressive for
  lifecycle close, not a strict pass-count regression-guard improvement.
- The old scalar immediate printer diagnostic is absent from the focused target
  cases and from the broader backend-regex proof.
- Focused residuals are outside the idea 299 closure boundary: `00031` passes;
  `00104` is invalid scalar cast spelling; `00213` and `00214` are
  symbol-store value printer residuals; `00207` and `00215` segfault; `00143`
  times out; and `00218` is a runtime mismatch.
- Remaining scalar cast, symbol-store value printing, runtime nonzero,
  runtime mismatch, timeout, and other parked backend-regex buckets should be
  classified through this umbrella before another focused owner is split.

Post-299 split 2026-05-19:

- Step 1 of the active umbrella runbook reconstructed the committed post-299
  backend-regex residual inventory from `test_before.log`: 352 selected tests,
  294 passed, and 58 failed. No fresh broad runtime rerun was performed during
  this lifecycle split.
- The best next focused owner is idea 300,
  `ideas/open/300_aarch64_scalar_cast_machine_printer_forms.md`, covering the
  machine-printer/frontend scalar-cast residuals `00035`, `00105`, `00126`,
  `00134`, `00135`, `00151`, and `00208`.
- The split is based on shared scalar-cast diagnostics: `zero_extend` requires
  supported integer source/result widths and `sign_extend` requires a
  structured register source before printing.
- Remaining symbol-store value printing, other machine-printer/frontend,
  semantic `lir_to_bir`, invalid operand, runtime nonzero, runtime mismatch,
  and timeout buckets remain parked under this umbrella for later
  classification or separate focused splits.
- Active implementation should move to idea 300 before code edits begin.

Focused owner closure 2026-05-19:

- Focused idea 300,
  `ideas/closed/300_aarch64_scalar_cast_machine_printer_forms.md`, is closed
  as complete for the AArch64 scalar-cast machine-printer forms owner.
- The accepted focused proof covers `00035`, `00105`, `00126`, `00134`,
  `00135`, `00151`, and `00208`; the old scalar-cast printer diagnostics for
  unsupported `zero_extend` forms and unstructured `sign_extend` sources are
  absent.
- Current focused residuals are runtime residuals, not scalar-cast printer
  blockers by current evidence: `00035` and `00151` are `RUNTIME_NONZERO
  exit=1`, and `00208` is `RUNTIME_NONZERO exit=Segmentation fault`.
- Broad backend-regex proof now reports 352 selected tests, 298 passed, and 54
  failed, with no old scalar-cast printer diagnostics in the broad log.
- Remaining frontend, backend, runtime nonzero, runtime mismatch/crash, and
  timeout buckets stay parked under this umbrella for later classification or
  focused splits. Do not reopen idea 300 without generated-code or diagnostic
  evidence that contradicts the scalar-cast printer closure boundary.

Post-300 split 2026-05-19:

- Step 1 of the active umbrella runbook reconstructed the committed post-300
  backend-regex residual inventory from `test_before.log`: 352 selected tests,
  298 passed, and 54 failed. No fresh broad runtime rerun was performed during
  this lifecycle split.
- The best next focused owner is idea 301,
  `ideas/open/301_aarch64_memory_store_operand_materialization.md`, covering
  the memory-store machine-printer/frontend residuals `00173`, `00176`,
  `00181`, `00182`, `00187`, `00194`, `00213`, and `00214`.
- The split is based on shared store operand diagnostics: memory store source
  scratch values are not printable for `00173`, `00187`, and `00194`, while
  symbol/global store values are not represented as register/immediate
  operands for `00176`, `00181`, `00182`, `00213`, and `00214`.
- Remaining scalar selected-node operand-shape gaps, call-boundary move forms,
  unsigned reduction/logical-shift-right gaps, semantic `lir_to_bir`
  local-memory handoff gaps, invalid scalar cast spelling, runtime nonzero,
  runtime mismatch/crash, and timeout buckets remain parked under this
  umbrella for later classification or separate focused splits.
- Active implementation should move to idea 301 before code edits begin.

Focused owner closure 2026-05-19:

- Focused idea 301,
  `ideas/closed/301_aarch64_memory_store_operand_materialization.md`, is
  closed as complete for the AArch64 memory-store operand materialization
  owner.
- The accepted closure proof rebuilt the default preset and reran the backend
  regex scope into `test_after.log`: 352 selected tests, 300 passed, and 52
  failed.
- Matching regression guard comparison against `test_before.log` passed in
  documented non-decreasing mode because the available baseline was already at
  the same 300/52 state; no new failing tests were introduced.
- The old memory-store operand printer diagnostics are absent from the focused
  memory-store cases and from the broader backend-regex proof.
- Current focused residuals are outside the idea 301 closure boundary by
  present evidence: `00173`, `00181`, and `00214` are runtime nonzero/segfault
  residuals; `00176` is a runtime mismatch; `00182` is a backend assembler
  immediate-encoding residual; and `00187` times out. `00194` and `00213`
  pass.
- Remaining runtime nonzero, runtime mismatch/crash, frontend/backend
  residuals outside the old store-printer mode, and timeout buckets stay parked
  under this umbrella for later classification or focused splits. Do not
  reopen idea 301 without generated-code or diagnostic evidence that
  contradicts the memory-store operand materialization closure boundary.

Post-301 split 2026-05-19:

- Step 2 of the active umbrella runbook classified the committed post-301
  backend-regex residual inventory from accepted `test_before.log`: 352
  selected tests, 300 passed, and 52 failed. No fresh broad runtime rerun was
  performed during this lifecycle split.
- The best next focused owner is idea 302,
  `ideas/open/302_aarch64_scalar_machine_node_operand_forms.md`, covering the
  scalar machine-node operand-form residuals `00064`, `00139`, and `00205`.
- The split is based on direct compile-stage diagnostics showing selected
  scalar `div`, scalar `mul`, and scalar `logical_shift_right` unsigned
  reduction nodes reaching AArch64 machine printing without structured
  operands the printer accepts.
- The focused owner is separate from closed fused compare-branch owner 296,
  scalar immediate owner 299, scalar-cast owner 300, and memory-store owner
  301.
- Assembly legality/materialization singletons `00104` and `00182`, the
  call-boundary move gap `00140`, `lir_to_bir` residuals `00204` and `00216`,
  runtime nonzero/mismatch/crash buckets, and timeout/output-storm cases remain
  parked under this umbrella until narrow probes justify separate owners.
- Active implementation should move to idea 302 before code edits begin.

Post-305 split 2026-05-19:

- Step 2 of the active umbrella runbook classified the supervisor-captured
  broad backend baseline from `test_before.log` plus existing generated
  AArch64 assembly under `build/c_testsuite_aarch64_backend/`.
- The best next focused owner is idea 306,
  `ideas/open/306_aarch64_symbol_offset_address_materialization_width.md`,
  covering AArch64 symbol+offset address materialization/register-width
  legality for `00050.c`, `00176.c`, and `00182.c`.
- The split is based on concrete assembler-legality evidence: generated
  `adrp` symbol+offset address formation uses a 32-bit `wN` temporary and then
  reuses that `wN` as a memory base before `ldr`/`str`.
- This owner is distinct from closed fused compare-branch operand forms,
  scalar immediate fallback, scalar-cast spelling, memory-store source
  materialization, and scalar selected-node operand forms; the current bad
  operation is address-register width selection for symbol+offset memory
  references after assembly has already been emitted.
- `00189.c` remains parked as an externally binding symbol/PIC relocation
  bucket unless future evidence proves the same repair owns GOT/PIC address
  formation for symbols such as `stdout`.
- Runtime nonzero, runtime mismatch/crash, timeout, output-storm,
  call-boundary move, scalar `mul` printable-rhs, unprepared frame-slot source,
  and semantic `lir_to_bir` residuals remain parked under this umbrella for
  later classification or separate focused splits.
- Active implementation should move to idea 306 before code edits begin.

Focused owner closure 2026-05-19:

- Focused idea 306,
  `ideas/closed/306_aarch64_symbol_offset_address_materialization_width.md`,
  is closed as the AArch64 symbol+offset address materialization/register-width
  legality owner.
- The accepted focused proof has been rolled forward to `test_before.log`, so
  the next umbrella pass should treat that log as the current baseline rather
  than reopening the closed owner from counts alone.
- The next lifecycle pass should re-inventory or classify remaining
  backend-regex buckets without reopening closed owners 285 through 306 unless
  generated-code or proof evidence contradicts their closure boundaries.
- Runtime nonzero, runtime mismatch/crash, timeout, output-storm,
  call-boundary move, scalar `mul` printable-rhs, unprepared frame-slot source,
  externally binding symbol/PIC relocation, and semantic `lir_to_bir`
  residuals remain parked under this umbrella until a narrow probe justifies a
  focused owner split.

Post-306 split 2026-05-19:

- Step 1 and Step 2 of the active umbrella runbook reconstructed and
  classified the accepted post-306 backend-regex inventory from
  `test_before.log` without rerunning tests: 352 selected, 306 passed, and 46
  failed.
- All 46 residual failures are `c_testsuite_aarch64_backend_*` tests; local
  backend/unit/CLI tests selected by `-R backend` passed.
- The best next focused owner is idea 307,
  `ideas/open/307_aarch64_large_scalar_immediate_materialization.md`, covering
  the AArch64 large scalar immediate assembler residual in `00182.c`.
- The split is based on concrete assembler-legality evidence: generated
  assembly emits `mov x0, #1234567`, which is not a legal single-instruction
  AArch64 immediate move form and should be materialized through legal scalar
  constant-lowering behavior.
- `00189.c` remains parked as a distinct externally binding symbol/PIC
  relocation bucket involving non-PIC relocation against `stdout`.
- Machine-printer/prepared-node residuals (`00140`, `00164`, `00214`),
  semantic `lir_to_bir` admission residuals (`00204`, `00216`), runtime
  nonzero, runtime mismatch/crash, output-storm, and timeout buckets remain
  parked under this umbrella for later classification or separate focused
  splits.
- Active implementation should move to idea 307 before code edits begin.

Focused owner closure 2026-05-19:

- Focused idea 307,
  `ideas/closed/307_aarch64_large_scalar_immediate_materialization.md`, is
  closed as complete for the AArch64 large scalar immediate materialization
  owner.
- Commit `4af6bc256` removed the old `00182.c` assembler-stage failure by
  reusing shared integer constant materialization for call-boundary scalar
  immediates; generated assembly no longer emits `mov x0, #1234567`.
- The accepted focused proof remains 1/2 because `00182.c` now fails as
  `RUNTIME_MISMATCH`, while `backend_aarch64_machine_printer` passes.
- Close-time non-decreasing regression guard on the focused canonical
  `test_before.log` / `test_after.log` pair passed with no new failing tests.
- The remaining `00182.c` runtime mismatch is outside the idea 307 closure
  boundary and returns to this umbrella's runtime mismatch bucket. Do not
  reopen idea 307 unless generated-code evidence shows illegal large scalar
  immediate forms still reach AArch64 assembly printing.

Post-307 split 2026-05-19:

- Step 2 of the active umbrella runbook classified the fresh post-307
  backend-regex capture from `test_before.log`: 352 selected, 306 passed, and
  46 failed.
- All 46 residual failures are `c_testsuite_aarch64_backend_*` tests; local
  backend/unit/CLI tests selected by `-R backend` passed.
- The best next focused owner is idea 308,
  `ideas/open/308_aarch64_extern_data_symbol_pic_address_formation.md`,
  covering AArch64 externally binding data-symbol/PIC-safe address formation
  for the `00189.c` `stdout` residual.
- The split is based on concrete linker and generated-assembly evidence:
  `00189.c.s` emits direct `adrp`/`:lo12:` address formation for `stdout`,
  and the linker rejects `R_AARCH64_ADR_PREL_PG_HI21` against the externally
  binding symbol `stdout@@GLIBC_2.17`.
- Runtime nonzero, runtime mismatch/crash, timeout/output-storm,
  machine-printer/prepared-node residuals (`00140`, `00164`, `00214`), and
  semantic `lir_to_bir` admission residuals (`00204`, `00216`) remain parked
  under this umbrella for later classification or separate focused splits.
- Active implementation should move to idea 308 before code edits begin.

Focused owner closure 2026-05-19:

- Focused idea 308,
  `ideas/closed/308_aarch64_extern_data_symbol_pic_address_formation.md`, is
  closed as complete for the AArch64 externally binding data-symbol/PIC-safe
  address formation owner.
- Commit `397c30c04` moved externally binding AArch64 data globals to
  `GotRequired` and taught the AArch64 `LoadGlobalInst` producer path to emit
  GOT page/low12 materialization for those globals while preserving direct
  local/internal data-global address formation in focused backend tests.
- The old `00189.c` failure mode,
  `R_AARCH64_ADR_PREL_PG_HI21` against `stdout@@GLIBC_2.17`, is gone; the
  generated assembly now uses `:got:stdout` / `:got_lo12:stdout`.
- The remaining `00189.c` result is `RUNTIME_NONZERO exit=Segmentation fault`
  and returns to this umbrella's runtime/call-argument classification path. Do
  not reopen idea 308 unless generated-code evidence shows externally binding
  data symbols again use direct non-PIC AArch64 relocation forms.

Post-308 split 2026-05-19:

- Step 2 of the active umbrella runbook compared the residual `00189.c`
  runtime segfault against runtime nonzero, crash, and mismatch neighbors using
  accepted umbrella history and current generated AArch64 artifacts. No fresh
  broad runtime rerun was performed during this lifecycle split.
- No broader shared semantic owner is ready by current evidence. Nearby
  call-boundary failures remain parked as distinct buckets: direct
  multi-argument shuffle around `00181.c` and `00182.c`, direct vararg
  aliasing around `00200.c`, and address-of-local direct-call argument
  preparation around `00218.c`.
- The focused owner split is idea 309,
  `ideas/open/309_aarch64_indirect_call_argument_preservation.md`, covering
  AArch64 indirect function-pointer callee and argument preservation across
  nested call setup for `00189.c`.
- The split is intentionally singleton but semantic: progress must preserve
  the indirect callee through argument setup, place outer-call arguments in the
  correct call registers after nested call setup, and avoid filename-specific
  or instruction-string-specific fixes.
- Runtime nonzero/mismatch/crash buckets outside this indirect-call shape,
  timeout/output-storm cases, direct-call shuffle and vararg buckets, and
  already parked machine-printer or semantic `lir_to_bir` residuals remain
  under this umbrella for later classification.
- Active implementation should move to idea 309 before code edits begin.

## Reviewer Reject Signals

Reject the route if it:

- fixes a named failing test without grouping the semantic owner;
- mixes backend unit failures and external AArch64 runtime failures without
  classification;
- uses expectation, allowlist, unsupported-classification, timeout, runner, or
  CTest registration changes to improve counts;
- reruns broad runtime tests without stale-process cleanup;
- reopens recently closed owners 285 through 308 without generated-code or
  proof evidence that contradicts their closure boundary.
