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

Focused owner closure 2026-05-19:

- Focused idea 309,
  `ideas/closed/309_aarch64_indirect_call_argument_preservation.md`, is
  closed as complete for AArch64 indirect-call callee and argument
  preservation.
- The accepted focused proof moved from 4/5 with
  `c_testsuite_aarch64_backend_src_00189_c` segfaulting in `test_before.log`
  to 5/5 passing in `test_after.log`; close-time regression guard passed on
  that matching focused scope.
- Supervisor broad local backend validation also passed
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` at 139/139.
- No residual from the focused indirect-call owner returns to this umbrella.
  Direct-call shuffle, direct vararg, address-of-local, timeout, runtime
  mismatch/crash, machine-printer/prepared-node, and semantic `lir_to_bir`
  residual buckets remain under this umbrella for later classification.
- The next lifecycle pass should re-inventory or classify the remaining
  backend-regex buckets without reopening closed owners 285 through 309 unless
  generated-code or proof evidence contradicts their closure boundaries.

Step 4 split 2026-05-19:

- Step 3 of the active umbrella runbook classified the fresh focused
  `test_after.log` evidence for `c_testsuite_aarch64_backend_src_00140_c`.
  That test fails at the AArch64 machine-node printer with
  `FRONTEND_FAIL` and the diagnostic `printer requires selected machine node,
  got deferred_unsupported: call-boundary move node is outside the selected
  register call-boundary move subset`.
- No `build/c_testsuite_aarch64_backend/src/00140.c.s` artifact exists, so
  this split is compile/printer-stage selected-node admission and prepared
  move publication, not emitted assembly spelling.
- The focused owner split is idea 311,
  `ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md`,
  covering AArch64 selected call-boundary move preparation, source/destination
  fact preservation, selected-node admission, and machine-printer consumption.
- The recommended proof scope is
  `backend_aarch64_target_instruction_records`,
  `backend_aarch64_machine_printer`,
  `backend_aarch64_instruction_dispatch`, and
  `c_testsuite_aarch64_backend_src_00140_c`.
- Remaining direct-call shuffle, direct vararg, address-of-local, runtime
  nonzero/mismatch/crash, timeout/output-storm, other machine-printer or
  prepared-node residuals such as `00164.c` and `00214.c`, and semantic
  `lir_to_bir` residuals remain parked under this umbrella until separate
  evidence justifies another focused split.
- Active implementation should move to idea 311 before code edits begin.

Focused owner closure 2026-05-19:

- Focused idea 311,
  `ideas/closed/311_aarch64_selected_call_boundary_move_preparation_printing.md`,
  is closed as complete for AArch64 selected call-boundary move preparation,
  source/destination fact preservation, selected-node admission, and
  machine-printer consumption.
- The accepted focused proof covered
  `backend_aarch64_target_instruction_records`,
  `backend_aarch64_machine_printer`,
  `backend_aarch64_instruction_dispatch`, and
  `c_testsuite_aarch64_backend_src_00140_c`; all four tests passed.
- The old generic selected-register-subset rejection and the intermediate
  stack-copy residual for `00140.c` are gone by the accepted focused proof.
- Close-time non-decreasing regression guard over the matching focused
  `test_before.log` / `test_after.log` pair passed with 4 passed before and 4
  passed after.
- No current `00140.c` residual returns to this umbrella. Remaining direct-call
  shuffle, direct vararg, address-of-local, runtime nonzero/mismatch/crash,
  timeout/output-storm, other machine-printer or prepared-node residuals such
  as `00164.c` and `00214.c`, and semantic `lir_to_bir` residuals remain
  parked until a later inventory pass justifies another focused split.
- The next lifecycle pass should re-inventory or classify the remaining
  backend-regex buckets without reopening closed owners 285 through 311 unless
  generated-code or proof evidence contradicts their closure boundaries.

Step 4 split 2026-05-19:

- Step 1 of the active umbrella runbook captured a fresh backend-regex
  inventory in `test_after.log`: 352 selected tests, 295 passed, and 57
  failed. All residual failures are `c_testsuite_aarch64_backend_*` tests.
- Step 2 classified the 57 residuals as 0 local backend/unit/CLI failures, 2
  frontend/prepared-node machine-printer diagnostics, 2 semantic `lir_to_bir`
  admission diagnostics, 33 runtime nonzero/crash cases, 16 runtime mismatch
  cases, 1 runtime output-storm plus crash, and 3 CTest timeouts.
- Step 3 selected the next focused owner as residual AArch64 semantic
  `lir_to_bir` local-memory admission/prepared-module handoff, represented by
  `c_testsuite_aarch64_backend_src_00204_c` and
  `c_testsuite_aarch64_backend_src_00216_c`.
- Focused idea 312,
  `ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md`, is now
  active. The proof scope is `backend_lir_to_bir_notes`, the existing
  `00204.c` semantic/prepared dump helpers, and the two representative
  c-testsuite cases.
- The split is semantic compile-stage work: neither `00204.c` nor `00216.c`
  has generated `.s` or `.bin` output in the current evidence, so this is not
  an AArch64 printer, assembler, linker, runtime, timeout, runner, or CTest
  registration owner.
- Remaining parked buckets include the machine-printer/prepared-node
  diagnostics such as `00164.c` and `00214.c`, runtime nonzero/crash cases,
  runtime mismatch cases, the runtime output-storm plus crash, timeout cases,
  direct-call shuffle, direct vararg, and address-of-local residuals.
- Active implementation should move to idea 312 before code edits begin. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked buckets.

Step 3/4 split 2026-05-20:

- The fresh backend-regex inventory in `test_after.log` selected 354 tests:
  299 passed and 55 failed. All current failures are
  `c_testsuite_aarch64_backend_*`; local backend/unit/CLI tests selected by
  the backend regex passed.
- The best current focused owner is idea 334,
  `ideas/open/334_aarch64_scalar_machine_node_operand_fact_printing.md`,
  covering AArch64 scalar machine-node operand fact preservation/printing for
  current compile/printer failures `00164.c` and `00214.c`.
- `00164.c` fails at selected scalar `mul` printing with incomplete printable
  RHS facts, while `00214.c` fails at selected scalar `add` printing because
  scalar add/sub/bitwise memory operands require prepared frame-slot sources.
- Fresh partial artifacts for those cases also show frame-size/prologue
  tension that may become relevant later: `00164.c` partially emits accesses
  above a `#64` prologue despite prepared `frame_size=256`, and `00214.c`
  partially emits accesses above a `#48` prologue despite prepared
  `frame_size=96`. This does not reactivate parked idea 316 yet because the
  current CTest first bad facts are printer diagnostics.
- Remaining buckets stay parked under this umbrella: runtime mismatch cases,
  runtime nonzero/crash cases, timeouts, aggregate/global initializer
  residuals, floating/math residuals, loop/control-flow residuals, and
  potential frame-layout residuals after scalar printer localization.
- Active implementation should move to idea 334 before code edits begin.

Step 3/4 split 2026-05-20:

- Step 2 classification from the active umbrella runbook found 53 current
  `c_testsuite_aarch64_backend_*` failures and 0 local backend/unit/CLI
  failures.
- The best current focused owner is idea 336,
  `ideas/open/336_aarch64_return_result_publication_epilogue_clobber.md`,
  covering the AArch64 return-result publication / epilogue-clobber bucket.
- The split owner covers 22 current failures: `00004`, `00011`, `00013`,
  `00014`, `00016`, `00019`, `00020`, `00022`, `00052`, `00087`, `00103`,
  `00112`, `00116`, `00117`, `00118`, `00121`, `00124`, `00139`, `00153`,
  `00159`, `00168`, and `00170`.
- Representative generated assembly shows simple scalar returns such as
  `00011.c.s`, `00022.c.s`, and `00052.c.s` loading the intended return value
  then overwriting `x0` with stale `x13`; pointer-local cases such as
  `00004.c.s` loading `w0` then overwriting `x0` with restored/stale `x20`;
  and call-result cases such as `00159.c.s`, `00168.c.s`, `00087.c.s`, and
  `00124.c.s` computing or preserving a result and then returning a restored
  or stale register.
- Remaining buckets stay parked under this umbrella: scalar conversion,
  comparison, FP, and bitfield value lowering; addressable memory, pointer
  slot, and indexed aggregate materialization; control-flow, switch, loop
  induction, and fallthrough lowering; call/ABI/varargs/libc and indirect-call
  pointer materialization; AArch64 scalar-cast machine-printer operand
  normalization; and timeout/output-storm residuals.
- Active implementation should move to idea 336 before code edits begin. Do
  not reopen closed owners 333 or 335 from this bucket without fresh
  generated-code evidence tying the current first bad fact to those exact
  closure boundaries.

Step 3/4 split 2026-05-20:

- Step 2 classification committed as `701516e60` found the post-337
  backend-regex residual surface at 354 selected tests, with 325 passed, 26
  failed, and 3 timed out.
- Local/internal backend tests selected by the backend regex are clean. The
  remaining residuals are external `c_testsuite_aarch64_backend_*` failures.
- The next focused owner is idea 338,
  `ideas/open/338_aarch64_scalar_cast_register_source_operand_facts.md`,
  covering the 9-case AArch64 scalar cast machine-printer operand-fact bucket:
  `00143`, `00173`, `00175`, `00176`, `00181`, `00185`, `00204`, `00205`,
  and `00216`.
- The split is based on direct printer diagnostics: selected scalar cast
  machine nodes for `sign_extend` and `zero_extend` reach the AArch64 printer
  without a structured register source operand.
- This is adjacent to closed idea 334's scalar `mul`/`add` operand-fact owner
  but is a distinct scalar cast register-source operand-fact boundary. Do not
  reopen closed owners 334 through 337 from counts alone.
- Runtime nonzero/crash, runtime mismatch, timeout/output-storm, and other
  parked buckets remain under this umbrella until a later classification pass
  justifies another focused split.
- Active implementation should move to idea 338 before code edits begin. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked buckets.

Step 4 split 2026-05-20:

- Step 3 of the active umbrella runbook used the post-338 `test_after.log`,
  generated AArch64 assembly, and prepared-BIR probes to reject a combined
  scalar integer/FP runtime-value owner as too broad.
- The selected focused owner is idea 339,
  `ideas/open/339_aarch64_scalar_local_storage_writeback_sizing.md`, covering
  AArch64 scalar local storage/writeback sizing for non-address-exposed scalar
  locals, initially scoped to `00086` and `00111`.
- The split is based on generated-code and prepared-access evidence: `00086`
  shows frame-size-zero short local access, missing increment writeback, and a
  stale reload for compare; `00111` reads short local `s` before
  initialization, computes `s - l`, does not write the result back, and
  returns the stale slot.
- Parked buckets remain separate: FP comparison result materialization
  (`00119`, `00123`), pointer/null conditional and pointer-local materialization
  (`00112`, `00144`), FP expression/call materialization (`00174`), and broad
  return-spill/ABI materialization (`00200`).
- Active implementation should move to idea 339 before code edits begin. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked buckets.

Step 4 split 2026-05-20 post-339:

- Step 3 of the active umbrella runbook used the post-339 focused evidence for
  `c_testsuite_aarch64_backend_src_00143_c` to select a reopened scalar-cast
  register-source publication owner.
- The representative fails before assembly at selected AArch64 machine-node
  printing: `family=scalar opcode=sign_extend`, with the diagnostic that a
  scalar cast node requires a structured register source operand.
- Prepared evidence shows the source path `%t76 = bir.select ... i16` feeding
  `%t81 = bir.sext i16 %t76 to i32`; `%t76 value_id=308` is stack-homed, while
  `%t81 value_id=388` is a register value. The move bundle before the failing
  selected instruction contains `move from_value_id=308 to_value_id=388
  destination_storage=register reason=consumer_stack_to_register`.
- This is the same scalar cast structured register-source first bad fact as
  closed idea 338, but narrowed to the post-regalloc selected-source
  publication path where the prepared consumer stack-to-register move exists
  and is not exposed to the selected scalar cast printer operand.
- This is not a local scalar storage/writeback sizing residual from closed idea
  339 by current first-bad-fact evidence.
- Focused idea 340,
  `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`,
  is now active. It references closed idea 338 as the reopened/split boundary
  without mutating the archive.
- Remaining runtime nonzero/crash, runtime mismatch, timeout/output-storm, FP
  comparison/materialization, pointer/null conditional, broad return/ABI, and
  other parked buckets remain under this umbrella until a later classification
  pass justifies another focused split.
- Active implementation should move to idea 340 before code edits begin. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked buckets.

Step 3 split 2026-05-20 post-345:

- Step 1 inventory commit `67736b8b2` and Step 2 classification commit
  `c47813313` classified the current backend-regex inventory from
  `/workspaces/c4c/test_after.log`: 354 tests selected, 330 passed, 24
  failed, and 2 timed out.
- Local backend/unit/CLI tests selected by the backend regex are clean. The
  remaining residuals are external `c_testsuite_aarch64_backend_*` failures.
- The best current focused owner is idea 346,
  `ideas/open/346_aarch64_direct_call_argument_formal_publication.md`,
  covering AArch64 direct-call argument/formal publication for `00140`,
  `00159`, `00170`, `00175`, and `00218`.
- The split is based on generated-code evidence that prepared direct-call
  operands and callee formal homes fail to publish to or consume the AAPCS64
  ABI argument registers/stack slots: stale `w20` instead of incoming `w0` in
  `00159`, stale `w13`/`d13` call values in `00175`, a missing eighth
  variadic/overflow integer argument in `00170`, uninitialized `x20` aggregate
  or address argument publication in `00140`, and uninitialized `x21` instead
  of `&convs` in `00218`.
- This is adjacent to closed ideas 309 and 311, but the current first bad fact
  is runtime ABI argument/formal publication, not an indirect-call
  preservation fault or selected call-boundary machine-printer diagnostic.
- Remaining buckets stay parked under this umbrella: pointer/null/scalar
  condition result publication, FP comparison/expression value
  materialization, broad addressable memory/indexed aggregate/pointer-local
  materialization, libc/file/string residuals, semantic `lir_to_bir` admission,
  and timeout/output-storm cases.
- Active implementation should move to idea 346 before code edits begin. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked buckets.

Step 4 switch 2026-05-20 post-347:

- The fresh backend-regex inventory selected 354 tests and classified the
  current residual surface down to 21 external AArch64 failures.
- The selected existing focused owner was closed adjacent owner idea 328,
  `ideas/closed/328_aarch64_byval_aggregate_call_argument_lane_publication.md`.
- The current first bad fact is again caller-side byval aggregate lane
  publication in `00204`: `fa_s1(s1)` reaches the call boundary with the
  address of the prepared byval temporary in `x0` (`add x0, sp, #928`) instead
  of packing the one-byte payload into `w0` as required by the callee and
  AAPCS64 integer argument lane classification.
- This is fresh generated-code evidence inside idea 328's existing scope and
  not a reopen from failing counts alone. Dynamic indexed aggregate/global
  access and FP expression/comparison lowering remain viable future owners but
  were not selected for this switch.
- Active implementation should move to idea 328 before code edits begin. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the remaining parked buckets.

Step 4 switch 2026-05-20 post-328:

- Step 3 of the active umbrella runbook selected focused owner idea 348,
  `ideas/open/348_aarch64_indexed_aggregate_address_writeback.md`, from the
  committed post-328 residual classification.
- The selected bucket is indexed aggregate addressing/writeback: `00130`,
  `00176`, `00182`, `00187`, `00195`, with `00181` as a nearby recursive
  pointer/global-array crash. The owner is based on generated-code first-bad
  facts, not failing counts.
- Representative evidence includes wrong local byte placement for `arr[1][3]`,
  unchanged global quicksort state after indexed swaps, lost LED buffer
  digits, a local buffer terminator landing at the wrong byte, repeated stale
  `d9` stores while walking `point_array+N`, and recursive global tower array
  mutation through fixed snapshots instead of selected element addresses.
- Existing parked aggregate ideas do not own this bucket by current evidence:
  their scopes are variadic, byval, `va_arg`, call-boundary, or publication
  boundaries rather than dynamic indexed aggregate address/writeback.
- Remaining buckets stay parked under this umbrella: boolean/comparison
  materialization, FP comparison/expression lowering, semantic admission,
  static aggregate initializer/relocation materialization, recursive scalar
  state, pointer indirection, loop/update state, runtime timeout/output-storm,
  and other unresolved runtime residuals.
- Active implementation should move to idea 348 before code edits begin. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the remaining parked buckets.

Step 3 route 2026-05-21 post-348/354:

- Step 1 captured the current backend-regex surface from
  `ctest --test-dir build -j10 -R backend --output-on-failure`: 354 selected,
  337 passed, 15 failed, and 2 timed out.
- Local backend/unit/CLI tests selected by the backend regex are clean. The
  remaining non-passing cases are external `c_testsuite_aarch64_backend_*`
  tests.
- Step 2 classified the dominant residual family as address-valued memory and
  stack-home publication confusion across indirect load/store and
  call-argument boundaries.
- The selected focused owner is idea 355,
  `ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md`,
  covering the semantic choice between materializing an address, reloading a
  pointer value, and loading or storing a pointee.
- Representative evidence includes `00020` returning an intermediate pointer
  address instead of the final `i32`, `00170` reloading `[sp, #8]` instead of
  materializing `sp+8` for an address-exposed local, `00189` reloading an
  unpublished `stdout` stack home before an indirect call, and `00173` pointer
  loops reloading fixed string addresses instead of advancing through current
  `*b` and `*src` values.
- Rejected adjacent owners remain parked under this umbrella for later
  classification: scalar compare/select publication, floating-point variadic
  scalar correctness, composite/byval/HFA ABI, dynamic stack/goto timeout, and
  complex aggregate initializer/relocation.
- Active implementation should move to idea 355 before code edits begin. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the remaining parked buckets.

Step 3/4 split 2026-05-21 post-365/366:

- Step 1 of the active umbrella runbook reconstructed the current
  backend-regex residual surface from the committed proof log after closed
  ideas 365 and 366: 354 selected tests, 333 passed, and 19 non-passing
  residuals. Local backend/unit/CLI tests selected by the backend regex are
  clean, and `c_testsuite_aarch64_backend_src_00173_c` passes.
- Step 2 classification committed as `a4093586e` selected semantic BIR
  indirect local-memory lvalue admission as the next focused owner.
- The focused split is idea 367,
  `ideas/open/367_semantic_bir_indirect_local_memory_lvalue_admission.md`,
  covering `lir_to_bir` lowering for indirect local-memory lvalues whose
  address comes from loaded pointer values, pointer-to-pointer locals, pointer
  arithmetic, or casts.
- Representative first bad facts are compile-stage semantic handoff failures:
  `c_testsuite_aarch64_backend_src_00005_c` rejects the store local-memory
  path for `**pp = 1`, and `c_testsuite_aarch64_backend_src_00217_c` rejects
  the load local-memory path for `*(unsigned*)(data + r) += a - b`.
- This owner is separate from the completed `00173` pointer-derived string
  load/publication chain, current local backend-route expectation drift,
  runtime scalar compare/select buckets, composite/byval/HFA/f128 ABI buckets,
  aggregate/writeback runtime buckets, timeout/output-storm buckets, and
  AArch64 printer or runner policy.
- Active implementation should move to idea 367 before code edits begin. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the remaining parked buckets.

Step 3 split 2026-05-21 post-368:

- Step 2 of the active umbrella runbook classified the current backend-regex
  residual surface into 26 non-passing residuals: 24 failed and 2 timed out.
- The selected focused owner is idea 369,
  `ideas/open/369_semantic_bir_local_pointer_memory_observation_canonicalization.md`,
  covering the 7-case semantic-BIR local/pointer memory observation bucket.
- The selected bucket fails before runtime as
  `BACKEND_ROUTE_SNIPPET_MISSING`, with actual semantic-BIR dumps substituting
  string-literal pointer values or computed-pointer load/store paths where the
  route observers expect local pointer stores and dynamic local
  aggregate/member lane materialization.
- Existing open or parked ideas do not own this route by current evidence:
  nearby pointer-derived string-load and pointer-addressed store/load ideas
  are parked after their source intent was satisfied, while the open
  variadic/HFA, formal-publication, and ABI ideas are generated-code or
  call-boundary routes rather than semantic-BIR observation failures.
- Remaining buckets stay parked under this umbrella: AArch64 scalar
  expression result materialization, aggregate/array/member address
  materialization, switch/fallthrough with pointer post-increment, composite
  ABI/HFA/f128 call-boundary preparation, libc call result materialization,
  complex initializer/relocation semantics, unsigned enum bitfield
  load/extension, and the two quarantined timeout residuals.
- Active implementation should move to idea 369 before code edits begin. This
  umbrella should proceed to lifecycle handoff rather than retaining
  implementation work under the inventory runbook.

Lifecycle switch 2026-05-21:

- Umbrella inventory idea 295 is parked after selecting focused owner idea 369,
  `ideas/open/369_semantic_bir_local_pointer_memory_observation_canonicalization.md`.
- The active lifecycle state should now use idea 369 for implementation. The
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.
- Remaining parked buckets are unchanged from the Step 3 split: AArch64 scalar
  expression result materialization, aggregate/array/member address
  materialization, switch/fallthrough with pointer post-increment, composite
  ABI/HFA/f128 call-boundary preparation, libc call result materialization,
  complex initializer/relocation semantics, unsigned enum bitfield
  load/extension, and the two quarantined timeout residuals.

Step 3 selection 2026-05-21 post-369:

- Step 2 classified the current post-369 backend-regex surface into 18
  external AArch64 residuals using `test_after.log`, source files, and
  generated assembly.
- The selected focused owner is existing idea 326,
  `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`, resumed for the
  current AArch64 aggregate/varargs ABI call-boundary lowering bucket.
- The selected bucket is represented by `00140` and `00204`: `00140` segfaults
  in calls involving a struct argument plus variadic extras, while `00204`
  fails in the machine printer with an explicit call-boundary move gap that
  requires prepared GPR registers, scalar FPR registers, or structured f128
  q-register authority.
- This selection uses fresh generated-code/diagnostic evidence and does not
  reopen prior variadic/HFA, byval, fixed-formal, stdarg cursor, MOVI, or
  local/value-home owners from pass counts alone.
- Remaining parked buckets stay under this umbrella for later classification:
  scalar comparison/value materialization, floating-point expression and
  vararg call lowering, aggregate/member lvalue address lowering, indexed
  array/select-matrix lowering, switch/select label ownership, conditional
  operator lowering, file API call-boundary result lowering, constant
  `sizeof` materialization, complex aggregate initializer/object layout,
  unsigned enum bit-field layout, and the two timeout-only residuals.

Lifecycle switch 2026-05-21 post-369:

- Umbrella inventory idea 295 is parked after selecting focused owner idea
  326, `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`.
- The active lifecycle state should now use idea 326 for implementation. The
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.
- Remaining parked buckets are unchanged from the Step 3 selection:
  scalar comparison/value materialization, floating-point expression and
  vararg call lowering, aggregate/member lvalue address lowering, indexed
  array/select-matrix lowering, switch/select label ownership, conditional
  operator lowering, file API call-boundary result lowering, constant
  `sizeof` materialization, complex aggregate initializer/object layout,
  unsigned enum bit-field layout, and the two timeout-only residuals.

Step 3 selection 2026-05-21 post-331:

- Step 2 classified 12 current non-timeout AArch64 backend residuals using
  `test_after.log` and generated artifacts after commit `61a786326`.
- The selected focused owner is new idea 364,
  `ideas/open/364_aarch64_synthetic_select_label_uniqueness.md`, for the
  `00143` compile/assembler failure caused by duplicate generated synthetic
  select labels such as `.Lselect_mat_1_24_164_37_true` and its `_end` label.
- Existing open ideas do not exactly own this bucket by current evidence:
  `ideas/open/352_aarch64_block_label_emission_ordering.md` is adjacent basic
  block label/epilogue ordering work, while this fact is duplicate synthetic
  select/materialized-label allocation inside emitted AArch64 assembly; the
  old `00143` scalar-cast source-publication thread is likewise not the
  current owner.
- Remaining non-timeout buckets stay parked under this umbrella for later
  classification: pointer constant comparison result publication, indexed
  local/global aggregate element writeback, local pointer reassignment
  writeback, scalar FP expression materialization, unsigned div/rem or local
  array digit publication, conditional operator/select value materialization,
  libc file-call result publication, constant `sizeof` or global aggregate
  metadata, complex aggregate initializer/object layout, and unsigned enum
  bit-field storage/load layout. `00200` and `00207` remain timeout-only
  quarantine cases.

Lifecycle switch 2026-05-21 post-331:

- Umbrella inventory idea 295 is parked after creating focused owner idea 364,
  `ideas/open/364_aarch64_synthetic_select_label_uniqueness.md`.
- The active lifecycle state should now use idea 364 for implementation. The
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.

Step 3 selection 2026-05-21 post-365:

- Step 2 classified 11 current non-timeout AArch64 backend residuals using
  `test_after.log` and generated artifacts after commit `e5aae0994`.
- The selected focused owner is new idea 366,
  `ideas/open/366_aarch64_string_literal_pointer_null_comparison.md`, for the
  `00112` runtime failure where `return "abc" == (void *)0;` lowers to
  `mov x0, x13; ret` and returns a stale register instead of materializing the
  string-literal pointer/null comparison result.
- Existing open ideas do not exactly own this bucket by current evidence:
  `ideas/open/356_semantic_bir_pointer_derived_string_loads.md` is a parked
  dynamic pointer-derived byte-load owner, not direct string-literal pointer
  constant comparison/result publication with no byte load.
- Remaining non-timeout buckets stay parked under this umbrella for later
  classification: indexed local/global aggregate element writeback, local
  pointer reassignment writeback, scalar FP expression materialization,
  unsigned div/rem or local/global element materialization, conditional/select
  expression producer materialization, libc/file API return-count publication,
  global array `sizeof`/loop-bound constant materialization, complex aggregate
  initializer/object layout, and unsigned enum bit-field storage/load layout.
  `00200` and `00207` remain timeout-only quarantine cases.

Lifecycle switch 2026-05-21 post-365:

- Umbrella inventory idea 295 is parked after creating focused owner idea 366,
  `ideas/open/366_aarch64_string_literal_pointer_null_comparison.md`.
- The active lifecycle state should now use idea 366 for implementation. The
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.

Step 3/4 selection 2026-05-21 post-370:

- Step 1 commit `1d1a1a3d2` captured the current backend-regex surface after
  closed ideas 366 and 370: 356 selected tests, 344 passed, and 12 failed.
  Local backend/unit/route/MIR/BIR/CLI/runtime/smoke tests are clean; all
  residuals are external `c_testsuite_aarch64_backend_*`.
- Step 2 commit `65b676c45` classified the leading residual families. The
  strongest current implementation owner is the dynamic indexed local/global
  aggregate address/writeback regression represented by `00157` and `00176`.
- This bucket overlaps closed idea 348, but current generated assembly gives
  fresh first-bad-fact evidence after that closure: `00157` snapshots a local
  array through fixed/high stack slots instead of storing through
  `Array[Count-1]`, while `00176` again expands global `array[...]` through
  large select/snapshot chains and stack temporaries, producing an unsorted
  array.
- Plan-owner selected a new follow-up owner rather than mutating the closed
  348 archive: `ideas/open/371_aarch64_indexed_aggregate_snapshot_writeback_regression.md`.
- Remaining ranked buckets stay parked under this umbrella for later
  classification: scalar FP constant/expression materialization (`00174`),
  conditional/switch selected arm materialization (`00183`, `00182`),
  external/libc call return publication (`00187`), pointer/subobject address
  publication (`00163`), global `sizeof`/aggregate metadata (`00205`),
  complex initializer/relocation (`00216`), unsigned enum bit-field layout
  (`00218`), and timeout quarantine (`00200`, `00207`).
- The active lifecycle state should now use idea 371 for implementation. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.

Step 3/4 selection 2026-05-21 post-371:

- Step 1 commit `ef3a8cc94` captured the current backend-regex surface after
  closed idea 371: 356 selected tests, 347 passed, and 9 failed. Local
  backend/unit/route/MIR/BIR/CLI/runtime/smoke tests are clean; all residuals
  are external `c_testsuite_aarch64_backend_*`.
- Step 2 commit `0d2bea3a4` classified the remaining residuals. The selected
  focused owner is new idea 372,
  `ideas/open/372_aarch64_pointer_valued_subobject_address_publication.md`.
- The selected representative is `00163`: generated AArch64 keeps using the
  old local pointer home for `&a` after `b = &(bolshevic.b)`, so the final
  `*b` prints `42` instead of the global member value `34`. This is fresh
  scalar pointer-local publication evidence, not a count-only reopen of closed
  pointer-derived address/lvalue owner 294 or address-valued call-argument
  owner 355.
- Remaining ranked buckets stay parked under this umbrella for later
  classification: external/libc call return publication (`00187`), static
  local/global selected array store/readback (`00182`), aggregate
  initializer/layout (`00205`, `00216`), scalar FP publication (`00174`),
  unsigned enum bit-field layout/address publication (`00218`), and timeout
  quarantine (`00200`, `00207`).
- The active lifecycle state should now use idea 372 for implementation. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.

Step 3/4 selection 2026-05-21 post-372:

- Step 1 refreshed the backend-regex surface after closed idea 372: 356
  selected tests, 348 passed, and 8 failed. Local backend/unit/route/MIR/BIR,
  CLI, runtime, and smoke tests are clean; all residuals are external
  `c_testsuite_aarch64_backend_*`.
- Step 2 commit `6a817e9dc` classified the remaining residuals. The selected
  focused owner is new idea 373,
  `ideas/open/373_aarch64_static_global_selected_value_publication.md`.
- The lead representative is `00182`: `print_led` stores extracted digits into
  the static local digit array, then uses selected-load chains for `d[i]`
  before calling `topline`, `midline`, and `botline`; the current runtime
  output renders all LED digits as zeroes. This is current selected
  static/global aggregate value publication before scalar consumers, not the
  old unsigned div/rem, large-immediate, frame-layout, or selected false-value
  closure boundary.
- `00205` and `00216` remain adjacency checks only until the lead selected
  value publication boundary is localized: `00205` also has a crisp scalar
  constant-binary stack publication first bad fact for loop bounds, and
  `00216` is broad aggregate initializer/relocation plus selected global
  function-pointer load stress.
- Remaining parked buckets stay under this umbrella for later classification:
  scalar constant-binary stack publication (`00205` first bad fact),
  external/libc call-result stack publication (`00187`), scalar FP
  arithmetic/materialization (`00174`), timeouts (`00200`, `00207`), and
  unsigned enum bit-field/local aggregate address publication (`00218`).
- The active lifecycle state should now use idea 373 for implementation. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.

Step 3/4 selection 2026-05-21 post-373:

- Step 1 refreshed the backend-regex surface after closed idea 373: 356
  selected tests, 349 passed, and 7 failed. Local backend/unit/route/MIR/BIR,
  CLI, runtime, and smoke tests are clean; all residuals are external
  `c_testsuite_aarch64_backend_*`.
- Step 2 commit `60d4641be` classified the remaining residuals. The selected
  focused owner is new idea 374,
  `ideas/open/374_aarch64_local_aggregate_address_call_publication.md`.
- The lead representative is `00218`: `main` should pass `&convs` to
  `convert_like_real`, and the callee's bit-field mask/compare is already
  plausible, but generated AArch64 passes stale `x21` instead of
  materializing the local aggregate address.
- `00216` is a crash guard with the same first crash shape in `foo`: the first
  `print(ls)` call passes stale `x13` instead of the address of local
  aggregate `ls`. Later aggregate initializer/function-pointer residuals
  should be reclassified after the local-address crash is repaired.
- Remaining parked buckets stay under this umbrella for later classification:
  scalar constant/`sizeof` stack-home publication (`00205`), external call
  result publication (`00187`), scalar FP publication (`00174`), dynamic
  stack/VLA fixed-slot timeout (`00207`), and shift/type-promotion timeout
  (`00200`).
- The active lifecycle state should now use idea 374 for implementation. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.

Step 3/4 selection 2026-05-21 post-375:

- Step 2 classified the current backend-regex residual surface from existing
  `test_after.log`, generated AArch64 assembly, and source representatives:
  6 external AArch64 failures remain after closed local aggregate bit-field
  layout owner 375.
- The selected focused owner is new idea 376,
  `ideas/open/376_aarch64_sizeof_loop_bound_stack_home_publication.md`.
- The lead representative is `00205`: source loop bounds are compile-time
  constants derived from `sizeof(cases) / sizeof(cases[0])` and
  `sizeof(cases->c) / sizeof(cases->c[0])`, but generated AArch64 compares
  loop indices against uninitialized stack homes loaded from `[sp, #40]`,
  `[sp, #32]`, `[sp, #64]`, and `[sp, #56]`. The emitted `cases:` global data
  is present, and the runtime prints no rows, so the first bad fact is before
  selected global element publication.
- Existing open ideas do not exactly own this bucket by current evidence.
  Nearby stack-home, aggregate, call-boundary, and pointer-publication ideas
  are adjacent but do not cover compile-time scalar `sizeof` loop-bound homes
  being consumed before publication.
- Remaining ranked buckets stay parked under this umbrella for later
  classification: external/libc call-result publication (`00187`), scalar FP
  expression or constant materialization (`00174`), aggregate
  initializer/compound relocation/function-pointer-table behavior (`00216`),
  dynamic stack/VLA fixed-slot timeout (`00207`), and shift/type-promotion
  timeout (`00200`).
- The active lifecycle state should now use idea 376 for implementation. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.

Step 3/4 selection 2026-05-21 post-376:

- Step 2 classified the current backend-regex residual surface from existing
  `test_after.log`, source representatives, and generated artifacts under
  `build/c_testsuite_aarch64_backend/`.
- The captured backend subset reports 357 selected tests, 352 passed, and 5
  residual failures. Local backend/unit tests selected by the backend regex
  remain clean; all residuals are external `c_testsuite_aarch64_backend_*`.
- The selected focused owner is new idea 377,
  `ideas/open/377_aarch64_external_libc_call_result_publication.md`.
- The lead representative is `00187`: source checks
  `fread(freddy, 1, 6, f) != 6`, but generated AArch64 calls `fread` and then
  compares stale `[sp, #96]` against `6` instead of the return count in `x0`.
  The runtime prints `couldn't read fred.txt` before otherwise reading the
  file successfully.
- Existing open ideas do not exactly own this bucket by current evidence.
  Recursive/call-preservation, direct argument/formal publication,
  local/formal frame-slot publication, scalar ALU producer publication, and
  address/pointer publication ideas are adjacent, but they do not cover
  external/libc call return-count publication to the immediate scalar
  comparison consumer.
- Remaining ranked buckets stay parked under this umbrella for later
  classification: scalar FP expression or constant materialization (`00174`),
  aggregate initializer/compound relocation/function-pointer-table behavior
  (`00216`), dynamic stack/VLA fixed-slot timeout (`00207`), and
  shift/type-promotion timeout (`00200`).
- The active lifecycle state should now use idea 377 for implementation. This
  umbrella should be reactivated only for a later classification pass or for
  splitting another focused owner from the parked residual buckets.

## Reviewer Reject Signals

Reject the route if it:

- fixes a named failing test without grouping the semantic owner;
- mixes backend unit failures and external AArch64 runtime failures without
  classification;
- uses expectation, allowlist, unsupported-classification, timeout, runner, or
  CTest registration changes to improve counts;
- reruns broad runtime tests without stale-process cleanup;
- reopens closed focused owners without generated-code or proof evidence that
  contradicts their closure boundary.
