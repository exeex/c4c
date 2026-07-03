Status: Active
Source Idea Path: ideas/open/571_rv64_inline_asm_carrier_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rerun The Four Carrier Representatives

# Current Packet

## Just Finished

Step 4: Rerun The Four Carrier Representatives is complete.

Reran the four RV64 gcc_torture inline asm carrier representatives through
`--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, and the RV64 object-route
CMake runner. Evidence was saved under
`build/agent_state/571_rv64_inline_asm_carrier_lowering/`, including
`commands.sh.txt`, per-case logs, `summary.md`, and `classification.tsv`.

Disposition:

- `src/20071211-1.c`: lowered; object route passed.
- `src/pr51933.c`: still unsupported with the narrower
  `unsupported_inline_asm_fragment` diagnostic for the incomplete unsupported
  constraint carrier.
- `src/pr56982.c`: inline asm carrier no longer blocks object compilation; the
  runner now reaches `RV64_BACKEND_RUNTIME_MISMATCH` with c4c segfaulting,
  which is after the inline-asm carrier compile path.
- `src/pr78438.c`: lowered; object route passed.

No representative still fails first through the old generic
`unsupported_instruction_fragment` inline asm carrier path.

## Suggested Next

Execute Step 5: Review And Close Readiness. Confirm the source acceptance
criteria against the focused backend tests, the Step 4 representative evidence,
and the unchanged non-goal surfaces, then prepare plan-owner close-readiness
review if appropriate.

## Watchouts

- This plan is limited to RV64 inline asm carrier calls.
- Do not implement general call ABI lowering, pointer arithmetic, select,
  floating-point binary, or branch-published phi lowering here.
- Do not add filename-specific matching for `src/20071211-1.c`,
  `src/pr51933.c`, `src/pr56982.c`, or `src/pr78438.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- `src/pr51933.c` remains intentionally diagnostic-only for the `imr` unsupported
  constraint carrier and should not be converted through named-case matching.
- `src/pr56982.c` no longer demonstrates an inline-asm compile blocker in this
  evidence pass; its remaining runtime mismatch/segfault should not be claimed
  as fixed by this plan.
- The positive emission path intentionally supports only complete no-result
  empty-template `~{memory}` carriers. Unsupported inline asm forms must stay on
  precise inline-asm-specific diagnostics rather than the old generic fallback.

## Proof

Build proof:

`cmake --build --preset default > test_after.log 2>&1`

Result: passed. Log path: `test_after.log`.

Representative rerun proof:

- `build/agent_state/571_rv64_inline_asm_carrier_lowering/commands.sh.txt`
- `build/agent_state/571_rv64_inline_asm_carrier_lowering/summary.md`
- `build/agent_state/571_rv64_inline_asm_carrier_lowering/classification.tsv`
- per-case dump/object-route logs under
  `build/agent_state/571_rv64_inline_asm_carrier_lowering/src_*/`

Additional checks:

- `git diff --check -- todo.md` passed.
- `rg -n '[ \t]+$' todo.md
  build/agent_state/571_rv64_inline_asm_carrier_lowering/summary.md
  build/agent_state/571_rv64_inline_asm_carrier_lowering/classification.tsv`
  found no trailing whitespace.
