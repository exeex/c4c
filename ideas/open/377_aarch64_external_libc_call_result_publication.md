# AArch64 External Libc Call-Result Publication

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair AArch64 generated-code publication for scalar return values from
external/libc calls so the call result in the ABI return register is consumed
by the next scalar comparison, store, or branch instead of stale stack storage.

## Why This Exists

Idea 295 classified the post-376 backend-regex residual surface and selected
`00187` as the clearest remaining non-timeout owner. In generated
`00187.c.s`, the source check `fread(freddy, 1, 6, f) != 6` lowers to a call
to `fread`, but the following comparison reads stale `[sp, #96]` instead of
the return count in `x0`. The runtime then prints `couldn't read fred.txt`
even though later file contents can be read.

Nearby open ideas cover recursive call preservation, direct argument/formal
publication, local/formal frame-slot publication, scalar ALU producer
publication, and pointer/address publication. None exactly owns the boundary
where an external libc call return value must be published to a scalar
consumer immediately after the call.

## In Scope

- Localize how prepared scalar call-result values from external calls are
  represented after AArch64 `bl` emission.
- Repair publication from AAPCS64 return registers such as `x0`/`w0` into the
  selected scalar value consumed by comparisons, stores, branches, or local
  homes.
- Cover file/libc call return-count shapes such as `fread` returning `size_t`
  before an integer comparison.
- Add or extend focused backend coverage for external call-result publication
  before using `00187` as external representative proof.

## Out Of Scope

- Recursive caller-clobbered argument preservation, direct call argument/formal
  publication, fixed-formal entry publication, and local/formal frame-slot
  publication unless fresh evidence proves this exact return-result owner
  depends on them.
- Return-value epilogue clobbering from functions returning to their callers;
  this idea is about consuming a callee return value inside the current
  function after an external call.
- Scalar FP expression materialization, aggregate initializer/relocation,
  dynamic stack/VLA timeout, shift/type-promotion timeout, runner behavior,
  CTest registration, timeout policy, expectation changes, unsupported
  classifications, or proof-log policy.
- Fixing only `00187`, only `fread`, one stack offset such as `[sp, #96]`, one
  register name, or one emitted instruction neighborhood.

## Acceptance Criteria

- The first bad fact is localized to a concrete call-result publication
  boundary after an external/libc `bl`.
- Focused backend coverage fails without the repair and passes with it for a
  scalar external-call result consumed by a comparison, store, or branch.
- `00187` advances past the stale post-`fread` stack-home comparison against
  `6`, or is reclassified by a new first bad fact after the return count is
  consumed from the published call result.
- Supervisor-selected guardrails for adjacent call argument, function return,
  local/formal, scalar producer, address publication, and file/string runtime
  paths remain stable.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00187`, `fread`, the constant `6`, `[sp, #96]`, one return
  register, one generated function, or one emitted `bl` neighborhood instead
  of repairing general scalar external-call result publication;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, diagnostic text, classification-only
  notes, or generated-text reshuffling while the post-call scalar consumer can
  still read stale stack storage instead of the `x0`/`w0` call result;
- broadens into recursive argument preservation, direct argument/formal
  publication, function return epilogue clobbering, scalar FP materialization,
  aggregate relocation, or timeout work without fresh first-bad-fact evidence;
- proves only the external `00187` representative while nearby focused
  backend coverage for scalar external-call result consumption is absent.
