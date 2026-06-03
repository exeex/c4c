# AAPCS64 Va Arg Payload Shape Authority

## Goal

Define the authority for AAPCS64 `va_arg` payload ABI and aggregate/HFA shape
facts so prealloc does not need to infer semantic payload shape from lowered
load names, slot suffixes, or memory access patterns.

## Why This Exists

The call ABI authority audit found that scalar AAPCS64 `va_arg` planning uses
the BIR placeholder call `return_type` and recomputes payload ABI locally with
`infer_call_arg_abi`. Aggregate `va_arg` planning consumes BIR size/align and
memory carrier facts, but HFA shape is inferred in
`variadic_entry_plans.cpp::infer_aapcs64_hfa_va_arg_shape` from post-call
loads, aggregate slot-name suffixes, addressing facts, and frame slots.

This is the clearest proven missing-fact overlap from the audit: BIR carries
fixed-call HFA lane facts, but variadic aggregate payload shape has no explicit
target-neutral or documented helper-local contract.

## In Scope

- Decide whether BIR runtime `va_arg` placeholders should carry explicit
  payload ABI and HFA/aggregate shape metadata.
- If BIR does not own the fact, document the narrow AAPCS64 helper-local
  reconstruction contract and constrain it to the intended placeholder route.
- Update variadic entry planning to consume the chosen contract.
- Add focused proof for scalar `va_arg`, aggregate `va_arg`, and HFA aggregate
  `va_arg` behavior on AAPCS64.

## Out Of Scope

- Reworking fixed-argument AArch64 HFA pressure.
- Changing non-AAPCS64 variadic entry behavior unless required by shared
  interfaces.
- Generalizing all aggregate call ABI facts beyond the `va_arg` payload shape
  boundary.
- Changing target ABI behavior or weakening existing variadic tests.

## Acceptance Criteria

- The scalar `va_arg` payload ABI route has a named authority and does not rely
  on silent type-only reconstruction unless that path is explicitly documented.
- Aggregate/HFA `va_arg` lane count, lane size, and destination-home semantics
  are carried or reconstructed through a documented narrow contract.
- Proof covers AAPCS64 scalar, aggregate, and HFA `va_arg` cases affected by
  the old inference.

## Reviewer Reject Signals

- The patch adds more slot-name or testcase-shaped parsing to infer HFA shape.
- Fixed-call HFA pressure is changed to mask the variadic payload contract gap.
- The route proves only one narrow va_arg fixture while aggregate or HFA cases
  remain unsupported or unexamined.
- Expectations are weakened, marked unsupported, or rewritten instead of
  repairing or documenting the authority boundary.
- The implementation expands into unrelated variadic or AArch64 call rewrites.

## Close Note

Closed on 2026-06-03.

The source idea is complete. AAPCS64 runtime `va_arg` placeholders now publish
explicit payload ABI metadata for scalar and aggregate payloads, and HFA
aggregate placeholders publish explicit lane count and lane size metadata.
Prealloc variadic entry planning consumes those facts instead of silently
deriving semantic payload shape from lowered load names, slot suffixes, or
incidental frame layout.

The retained narrow helper-local behavior is destination-home mapping for HFA
lanes. That mapping remains constrained to the aggregate `va_arg` access-plan
route; lane count and lane size come from explicit BIR payload metadata, and
mismatched homes fail closed as a missing aggregate access plan.

Proof status: final backend validation passed with `169/169` backend tests, and
the close-time backend regression guard passed with `169/169` before and after,
no new failures, and no resolved failures. Coverage includes scalar GP and FP
payload ABI, non-HFA aggregate payload shape and destination-home behavior, and
HFA aggregate lane count and lane size.
