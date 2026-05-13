Status: Active
Source Idea Path: ideas/open/217_c4cll_debug_flags_document_aarch64_asm_output.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Update the Debug-Flags Skill Recipes

# Current Packet

## Just Finished

Step 2 updated `.codex/skills/c4cll-debug-flags/SKILL.md` with the landed
AArch64 `.s` output route.

Documented recipe coverage:

- exact implemented command shape:
  `c4cll --codegen asm --target aarch64-linux-gnu input.c -o out.s`
- repo-local build form using `./build/c4cll`
- external assembler/link smoke using `aarch64-linux-gnu-as` and `clang
  --target=aarch64-linux-gnu`, aligned with the CMake helper split
- the distinction between semantic BIR, prepared BIR, MIR route visibility,
  backend-native machine-node `.s` printer output, and LLVM-route output
- the explicit boundary that `.s` output is not an internal assembler, object
  encoder, linker, or bridge that parses assembly back into backend semantics

## Suggested Next

Supervisor should review and commit this documentation-only Step 2 slice, then
decide whether the active lifecycle state is complete.

## Watchouts

- External execution still depends on local cross-toolchain availability; the
  skill documents the recipe but does not require this packet to run the
  external smoke.
- `review/aarch64_allocation_record_step2_review.md` remains untouched.

## Proof

No build required for this documentation-only packet.

Ran:

```bash
git diff --check -- .codex/skills/c4cll-debug-flags/SKILL.md todo.md
rg -n -- 'c4cll --codegen asm --target aarch64-linux-gnu input\.c -o out\.s' .codex/skills/c4cll-debug-flags/SKILL.md todo.md
rg -n -- 'tests/backend/case/aarch64_return_zero_smoke\.c' .codex/skills/c4cll-debug-flags/SKILL.md
rg -n -- 'not an internal assembler|not .*semantic bridge|parses? .*\.s .*back|\.s output is not' .codex/skills/c4cll-debug-flags/SKILL.md todo.md
```

Results are written to `test_after.log`.

The checks passed: diff whitespace validation succeeded, the documented command
shape is present, the example path uses
`tests/backend/case/aarch64_return_zero_smoke.c`, and the route-boundary text
states `.s` output is not parsed back into backend semantics.
