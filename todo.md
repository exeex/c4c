Status: Active
Source Idea Path: ideas/open/318_rv64_byval_aggregate_call_abi.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Byval Aggregate ABI Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible todo skeleton for Step 1
of `plan.md`.

## Suggested Next

Start Step 1 by reproving `src/00140.c`, capturing BIR, prepared-BIR, emitted
RV64 assembly, clang link, and qemu evidence, then classify whether the first
bad mechanism is caller-side byval transport, callee-side byval home access,
floating aggregate lane handling, or a separate residual.

## Watchouts

- Do not special-case `src/00140.c`, `%p.f`, field names, or fixed stack-slot
  homes.
- Do not fake callee-local aggregate values; prove caller-side payload
  transport across the call boundary.
- Keep aggregate-local, function-pointer, external-call, and select/control
  repairs out of scope unless evidence proves a true dependency.

## Proof

Lifecycle-only activation. No build or tests were run.
