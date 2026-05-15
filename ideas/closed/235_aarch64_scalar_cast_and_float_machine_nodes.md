# AArch64 Scalar Cast And Float Machine Nodes

Status: Closed
Created: 2026-05-14

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

Current AArch64 instruction records can describe simple integer casts, and
prepared/register layers can classify ordinary integer and floating-point
value homes, but block dispatch only lowers BIR binary integer ALU
instructions. Cast instructions are classified as scalar but are not lowered
by dispatch, and the terminal printer does not print sign-extend,
zero-extend, truncate, floating-point arithmetic, or floating-point conversion
nodes.

The archived `cast_ops.md` and `float_ops.md` contain valid semantic families,
but their old `x0`/`s0`/`d0` accumulator bridge is not current ownership.

## Scope

- Route supported `bir::CastInst` records through AArch64 scalar lowering
  without bypassing prepared storage and register-placement authority.
- Add selected machine-node and printer support for simple integer casts.
- Add typed FP/SIMD machine-node families for `F32` and `F64` arithmetic and
  conversions, including explicit register-bank transitions where required.
- Keep binary128 casts and arithmetic delegated to the separate binary128
  soft-float route.

## Non-Goals

- Do not rebuild the archived accumulator-based cast or float operation
  surfaces.
- Do not merge GPR and FPR/SIMD allocation policy in AArch64 codegen.
- Do not lower F128 through scalar F64 paths.
- Do not add fixture-shaped operation shortcuts or expectation-only progress.

## Proof Direction

- A simple integer sign/zero/truncate cast dispatches to a selected machine
  node and prints the expected typed AArch64 instruction form.
- `F32` and `F64` add/sub/mul/div lower through explicit FP/SIMD operands
  rather than integer ALU nodes.
- Float/integer conversions use typed conversion nodes and structured
  register-file transition facts.

## Closure Notes

Closed: 2026-05-15

The active runbook completed the intended supported scalar cast and F32/F64
route:

- simple integer `SExt`, `ZExt`, and `Trunc` select from prepared value homes
  and storage facts into structured scalar cast records
- selected integer casts print typed terminal AArch64 forms from structured
  operands
- F32/F64 `Add`, `Sub`, `Mul`, and `Div` select from prepared FPR/SIMD
  register authority into structured scalar ALU records and print typed FP
  instruction forms
- supported `SIToFP`, `UIToFP`, `FPToSI`, `FPToUI`, `FPExt`, and `FPTrunc`
  select and print from explicit GPR/FPR source/result register and
  bank-transition facts
- F128/binary128, i128, pointer casts, bitcasts, missing register/storage
  authority, and wrong-bank facts remain fail-closed through prepared,
  selected-record, dispatch, or printer diagnostics

F128/binary128 and i128 behavior remain outside this closed route and should
stay delegated to their separate source ideas.

Close proof used the accepted full-suite regression artifact in
`test_before.log`: 3167/3167 tests passed.
