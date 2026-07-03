Status: Active
Source Idea Path: ideas/open/573_rv64_select_phi_select_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rehydrate Select Evidence And Reproduce

# Current Packet

## Just Finished

- Lifecycle activation created this execution state for Step 1 of `plan.md`.

## Suggested Next

- Delegate Step 1 to an executor: inspect the saved 570 select artifacts and
  reproduce the current `src/20030408-1.c` RV64 object-route behavior with the
  supervisor-selected proof command.

## Watchouts

- Do not treat `src/20030408-1.c`, `test1`, `logic.end.117`, or
  `%t126.phi.sel0` as part of the repair contract.
- Keep select lowering separate from same-module call, inline asm carrier,
  floating-point binary, pointer arithmetic, branch/CFG reconstruction, and
  runtime comparison work unless focused evidence proves the first owner moved.
- Expectation rewrites, unsupported-marker edits, allowlist changes, and
  diagnostic-only renames are not capability progress.

## Proof

- Lifecycle-only activation. No build or test proof was run.
