# AArch64 Static/Global Selected Value Publication

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair AArch64 lowering where a selected value loaded from a static or global
aggregate is not published to the scalar consumer that follows, such as a call
argument or indirect-call target.

## Why This Exists

The post-372 backend inventory selected static/global selected value
publication as the next focused owner. The current backend-regex surface has
356 selected tests, 348 passed, and 8 external AArch64 residuals; local
backend/unit/route/MIR/BIR/CLI/runtime/smoke tests are clean.

The lead representative is `00182`. Source `print_led` computes digits into a
static local array `d[MAX_DIGITS]`, then selects `d[i]` before calling
`topline`, `midline`, and `botline`. Generated AArch64 reaches selected-load
chains for the static digit array, but the runtime output renders all LED
digits as zeroes, indicating the selected static/global value is not
published correctly to the scalar call consumers.

Adjacency checks exist but are not the initial owner:

- `00205` uses a global aggregate `cases[]` and selected loads from it, but
  its first bad fact is currently scalar constant-binary stack publication for
  loop bounds, so it should not drive this owner until the lead publication
  boundary is localized.
- `00216` stresses aggregate initializers, relocations, and selected global
  function-pointer loads feeding indirect calls, but it is broad enough to
  remain an adjacency check until this owner proves the selected-value path.

This route is distinct from prior closed `00182` owners: large scalar
immediate materialization, frame-slot layout, unsigned div/rem producer
publication, nested select false-value materialization, indexed aggregate
snapshot/writeback, and pointer-local address publication.

## In Scope

- Localize how selected values from static locals and global aggregates are
  represented through BIR/prepared/MIR/AArch64 lowering.
- Trace selected static/global loads to their scalar consumers, especially
  fixed-arity call arguments and value publication before call boundaries.
- Repair the general publication path so selected aggregate values feed the
  intended scalar consumer instead of stale, zero, or fallback values.
- Add focused backend coverage for selected static/global aggregate value
  publication before scalar consumers without relying only on `00182`.
- Prove `c_testsuite_aarch64_backend_src_00182_c` advances past the current
  all-zero selected digit publication failure or passes.

## Out Of Scope

- Scalar constant-binary stack publication for `00205` loop bounds.
- External/libc call-result stack publication such as `00187`.
- Scalar FP expression/constant materialization such as `00174`.
- Timeout-specific work for `00200` or `00207`.
- Unsigned enum bit-field layout/local aggregate address publication such as
  `00218`.
- Broad aggregate initializer/relocation work in `00216` unless localization
  proves the same selected-value publication boundary.
- Reopening closed `00182` owners from failing counts alone.
- Fixing only `00182`, `print_led`, one static array name, one digit value,
  one call target, one register, one stack offset, or one emitted instruction
  sequence.

## Acceptance Criteria

- The current `00182` first bad fact is localized to a concrete selected
  static/global load, prepared value publication, MIR handoff, AArch64
  register/stack home, or call-argument consumer boundary.
- Focused backend coverage guards selected static/global aggregate value
  publication before scalar consumers without depending only on `00182`.
- `c_testsuite_aarch64_backend_src_00182_c` no longer fails because selected
  digit values collapse to zero before `topline`, `midline`, or `botline`.
- If `00182` still fails, `todo.md` records the new first bad fact and whether
  it remains in this owner or needs a lifecycle split.
- `00205` and `00216` are sampled only enough to classify whether their first
  bad fact shares this selected-value publication boundary or remains parked.
- The supervisor-selected proof scope introduces no new backend-regex
  failures and does not regress recent closed selected-address/publication
  owners.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00182`, `print_led`, `topline`, `midline`, `botline`, one
  static array, one digit, one call target, one register, one stack offset, or
  one emitted instruction sequence instead of repairing selected static/global
  value publication generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, emitted-text reshuffling, or
  classification-only notes while selected static/global values can still
  collapse to stale or zero values before scalar consumers;
- reopens closed `00182` owners without fresh generated-code evidence that
  contradicts their closure boundary;
- folds scalar constant-binary stack publication, external call-result
  publication, scalar FP, timeout, bit-field, or broad initializer/relocation
  work into this route without a fresh first-bad-fact handoff;
- proves only the external representative while leaving focused
  selected-value publication behavior unguarded.
