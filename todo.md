Status: Active
Source Idea Path: ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Local Operand Ownership

# Current Packet

## Just Finished

Lifecycle switch from idea 277 to idea 278. Idea 277's result-register owner
layer is repaired, and the remaining `[RUNTIME_NONZERO] exit=253` failure is
now tracked as local/operand materialization scope.

## Suggested Next

Execute Step 1: localize why the local `x = 4` value in `00003.c` is not
materialized as the left subtraction operand before AArch64 arithmetic
emission.

## Watchouts

- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, or CTest contracts.
- Do not add filename-specific handling for `00003.c`.
- Do not route proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Preserve the idea 277 result-register repair: returned integer expression
  values must remain in `w0`/`x0` before `ret`.
- Keep runtime-runner and route-diagnostic work out of this plan unless the
  supervisor opens a separate lifecycle item.

## Proof

Lifecycle-only switch. No code validation run by plan-owner.
