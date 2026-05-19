Status: Active
Source Idea Path: ideas/open/310_prepared_indirect_call_string_argument_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Existing Direct-Call String Argument Authority

# Current Packet

## Just Finished

Lifecycle switched away from idea 309 because Step 2 cannot be completed
inside AArch64 lowering without guessing missing string-constant facts.

The preserved blocker is: LIR has `%t2 = getelementptr ... @.str1` for the
outer indirect call argument, but BIR/prepared data publish that argument only
as an ordinary register source and do not provide a prepared string-constant
address materialization for `.str1`.

## Suggested Next

Execute Step 1 of `plan.md`: trace the existing direct-call string argument
materialization authority and identify the smallest BIR/prepared producer path
needed to give indirect calls the same semantic fact.

## Watchouts

- Do not repair this by guessing in AArch64 from `00189.c`, `stdout`,
  `fprintfptr`, `.str1`, argument index, assembly text, or one-string
  heuristics.
- Do not edit expectations, allowlists, unsupported classifications, runner
  behavior, timeout policy, CTest registration, proof logs, or test contracts.
- Do not expand this prerequisite into idea 309's AArch64 callee/register
  preservation; return to idea 309 only after the prepared string argument fact
  exists.

## Proof

Lifecycle-only repair. No compiler implementation or build proof was run.
