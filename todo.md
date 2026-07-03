Status: Active
Source Idea Path: ideas/open/571_rv64_inline_asm_carrier_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Inline Asm Carrier Representation

# Current Packet

## Just Finished

Activation created the runbook for
`ideas/open/571_rv64_inline_asm_carrier_lowering.md`.

## Suggested Next

Execute Step 1 by inventorying the BIR, prepared-BIR, and RV64 object-emission
representation of `llvm.inline_asm` carrier `CallInst` nodes, starting from the
570 diagnostic artifacts and `c4c-clang-tools` queries.

## Watchouts

- This plan is limited to RV64 inline asm carrier calls.
- Do not implement general call ABI lowering, pointer arithmetic, select,
  floating-point binary, or branch-published phi lowering here.
- Do not add filename-specific matching for `src/20071211-1.c`,
  `src/pr51933.c`, `src/pr56982.c`, or `src/pr78438.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Keep unsupported inline asm forms on precise inline-asm-specific diagnostics
  instead of the old generic fallback.

## Proof

Activation-only lifecycle work. Run
`git diff --check -- plan.md todo.md`
before committing.
