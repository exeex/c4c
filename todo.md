Status: Active
Source Idea Path: ideas/open/572_rv64_same_module_call_result_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Same-Module Call Representation

# Current Packet

## Just Finished

Activation created the runbook for
`ideas/open/572_rv64_same_module_call_result_lowering.md`.

## Suggested Next

Execute Step 1 by inventorying the BIR, prepared-BIR, and RV64
object-emission representation of ordinary same-module `CallInst` nodes with
GPR arguments and integer results, starting from the 570 diagnostic artifacts
and `c4c-clang-tools` queries.

## Watchouts

- This plan is limited to ordinary same-module RV64 call/result lowering.
- Do not treat `llvm.inline_asm` carriers as ordinary calls.
- Do not implement select, floating-point binary, pointer arithmetic,
  prepared-authority, broad ABI, or runtime-comparison work here.
- Do not add filename-specific matching for `src/20000412-2.c` or
  `src/20000622-1.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Keep unsupported ABI forms on precise call-specific diagnostics instead of
  the old generic fallback.

## Proof

Activation-only lifecycle work. Run
`git diff --check -- plan.md todo.md`
before committing.
