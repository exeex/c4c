# AArch64 Local Aggregate Address Call Publication

Status: Closed
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair AArch64 lowering where the address of a local aggregate is passed to a
scalar pointer call consumer but the call argument uses a stale scratch or
callee-saved carrier instead of materializing the local aggregate address.

## Why This Exists

The post-373 backend inventory selected local aggregate address publication as
the next focused owner. The current backend-regex surface has 356 selected
tests, 349 passed, and 7 external AArch64 residuals; local backend tests are
clean.

The lead representative is `00218`. Source `main` creates local aggregate
`union tree_node convs`, stores `AMBIG_CONV` into its bit-field, then calls
`convert_like_real(&convs)`. Prepared and generated callee evidence is
plausible: `convert_like_real` loads from `[x0,#16]`, masks with `255`, and
compares against `152`. The first bad fact is at the callsite: generated
AArch64 passes stale `x21` in `x0` instead of materializing the address of the
local aggregate `convs`.

`00216` is an adjacency/crash guard. Its first local aggregate call in `foo`,
`print(ls)`, sets the call pointer from stale `x13` instead of materializing
the address of local aggregate `ls`, causing an early segfault before later
aggregate initializer or function-pointer-table facts can be trusted.

This is adjacent to closed direct-call and address-valued publication owners,
but current evidence is a fresh, narrower local aggregate address publication
failure rather than a count-only reopen.

## In Scope

- Localize how address-of local aggregate values are represented when used as
  scalar pointer call arguments.
- Trace local aggregate homes through BIR/prepared/MIR/AArch64 lowering to the
  call-argument publication point.
- Repair the general AArch64 publication path so `&local_aggregate` call
  arguments materialize the aggregate address instead of using stale scratch
  carriers.
- Add focused backend coverage for local aggregate address call publication.
- Prove `c_testsuite_aarch64_backend_src_00218_c` advances past the stale
  call-argument first bad fact or passes, and sample `00216` as a crash guard.

## Out Of Scope

- Scalar constant/`sizeof` stack-home publication such as `00205`.
- External/libc call-result publication such as `00187`.
- Scalar FP expression/constant materialization such as `00174`.
- Dynamic stack/VLA fixed-slot timeout work such as `00207`.
- Shift/type-promotion timeout work such as `00200`.
- Bit-field storage/mask semantics after the local aggregate address is
  correctly passed.
- Broad aggregate initializer, compound literal, relocation, or selected
  function-pointer table work in `00216` beyond the first local-address crash.
- Reopening closed direct-call or address-valued publication owners from
  failing counts alone.
- Fixing only `00218`, `00216`, one local aggregate name, one function name,
  one register, one stack offset, or one emitted instruction sequence.

## Acceptance Criteria

- The current first bad fact is localized to a concrete local aggregate
  address, prepared call operand, MIR handoff, or AArch64 call-argument
  publication boundary.
- Focused backend coverage guards address-of local aggregate values feeding
  scalar pointer call arguments without relying only on `00218`.
- `c_testsuite_aarch64_backend_src_00218_c` no longer fails because
  `convert_like_real(&convs)` receives a stale or unmaterialized pointer.
- `00216` is sampled enough to classify whether its first `print(ls)` crash
  shares this owner or remains parked after a new first bad fact appears.
- The supervisor-selected proof scope introduces no new backend-regex
  failures and does not regress recent address-valued or direct-call
  publication guardrails.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00218`, `00216`, `convs`, `ls`, `convert_like_real`,
  `print`, one local aggregate type, one register, one stack offset, or one
  emitted instruction sequence instead of repairing local aggregate address
  call publication generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, emitted-text reshuffling, or
  classification-only notes while calls can still consume stale carriers for
  `&local_aggregate`;
- reopens closed direct-call or address-valued publication owners without
  fresh generated-code evidence that contradicts their closure boundary;
- folds scalar constants, external call-result publication, scalar FP,
  dynamic-stack timeout, shift-promotion timeout, bit-field semantics, or
  broad initializer/relocation work into this route without a fresh
  first-bad-fact handoff;
- proves only the external representative while leaving focused local
  aggregate address call publication behavior unguarded.

## Closure Note

Closed 2026-05-21 after implementation commit `f76455f64` repaired the local
aggregate address call publication boundary. Focused backend coverage guards
direct and zero-offset computed local aggregate address call operands, while
protecting `llvm.va_start.p0` and ABI `byval_copy` operands from the fallback.

The stale-address first bad facts are gone. `00218` now materializes
`&convs` before `convert_like_real`, and `00216` now materializes the first
local aggregate `print(...)` addresses. Both remaining representatives are
advanced residuals outside this owner: `00218` exposes a local
aggregate/bit-field layout-store mismatch, and `00216` reaches a later
compound-relocation/function-pointer-table residual.

Supervisor full-suite review matched the accepted baseline at `3352` passed /
`23` failed / `3375` total with no new failing tests. No baseline roll-forward
was accepted because the candidate had no pass-count improvement.
