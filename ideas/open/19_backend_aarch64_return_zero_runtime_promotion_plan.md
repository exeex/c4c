# AArch64 Return Zero Runtime Promotion Plan

## Status

Open as of 2026-03-30.

## Relationship To Roadmap

Follows:

- `ideas/open/__backend_port_plan.md`
- `ideas/closed/18_backend_aarch64_nested_param_member_array_plan.md`

## Goal

Promote the bounded zero-argument constant-return runtime seam after the recent nested aggregate slices: `tests/c/internal/backend_case/return_zero.c` should run through the AArch64 asm path instead of the default LLVM-IR path.

## Why This Is Separate

- the current AArch64 emitter already supports the minimal constant-return helper shape, but the runtime harness still leaves `return_zero` on the default LLVM-IR path
- this is a lifecycle-sized cleanup slice rather than a new lowering family, so it should stay isolated from larger follow-on backend ideas
- keeping the change separate preserves the one-mechanism-per-patch rule and avoids mixing trivial runtime promotion with broader backend feature work

## Candidate Case

- synthetic/backend capability already covered by the minimal constant-return emitter path
- runtime: `tests/c/internal/backend_case/return_zero.c`

## Validation Target

- confirm the existing AArch64 constant-return emitter path still covers the exact `return_zero` runtime LIR shape
- promote `tests/c/internal/backend_case/return_zero.c` to `BACKEND_OUTPUT_KIND=asm`
- run targeted backend/runtime validation plus the normal full-suite monotonic regression check

## Non-Goals

- new AArch64 lowering work beyond the existing constant-return slice
- new synthetic backend fixtures unless the current runtime LIR shape unexpectedly differs from the existing bounded matcher
- aggregate, variadic, pointer, or global-addressing follow-ons

## Execution Notes

- inspect the runtime-emitted LIR for `return_zero.c` before changing `tests/c/internal/InternalTests.cmake`
- if the runtime LIR diverges from the existing minimal constant-return matcher, stop and split a new narrow idea instead of expanding this one
- keep the patch limited to runtime promotion and validation unless a bounded matcher fix is strictly required

## Success Condition

This idea is complete when `backend_runtime_return_zero` passes with `BACKEND_OUTPUT_KIND=asm` and the full regression comparison remains monotonic with zero newly failing tests.
