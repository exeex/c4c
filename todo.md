Status: Active
Source Idea Path: ideas/open/319_aarch64_hfa_aggregate_argument_runtime.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Earliest Argument Corruption

# Current Packet

## Just Finished

Lifecycle transition complete. Idea 318 was closed because scalar ALU
immediate materialization advanced past the illegal `mov w9, #503808`
assembler failure, and the remaining `00204.c` failure is runtime argument
corruption outside that owner.

## Suggested Next

Execute Step 1 from `plan.md`: localize the earliest corrupted `Arguments:`
value and identify the owning AArch64 ABI argument classification or lowering
surface.

## Watchouts

- Start from the earliest observed `Arguments:` corruption, before `Return
  values:`, `stdarg:`, `MOVI:`, or `opi()` execute.
- Preserve prior-owner guardrails for ideas 314, 315, 317, and 318.
- Do not reopen scalar ALU immediate materialization unless generated-code
  evidence shows an illegal scalar ALU immediate still reaches executable
  code.
- Do not special-case `00204.c`, one output line, one function name, one
  register, or one stack slot.

## Proof

Close-time guard for idea 318 used existing focused `test_before.log` and
`test_after.log` with non-decreasing comparison:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed at 10/11 before and 10/11 after, with no new failures.
