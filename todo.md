Status: Active
Source Idea Path: ideas/open/797_lir_to_new_bir_final_coverage_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build the terminal disposition matrix

# Current Packet

## Just Finished

Completed plan Step 1 documentation packet by creating
`docs/lir_to_new_bir_final_coverage_convergence/terminal_disposition_matrix.md`.
The matrix records accepted terminal inputs, accepted no-change/evidence
dispositions, preserved open 795/796/821/822 scopes, closed 848/849/850 via
813, and closed 847 deletion evidence. It also records that closed 867's
selected direct-local `LirVaStartOp` destination `va_list` pointer authority
is already received by 734 Step 7.52 commit `a680b50e8`.

Step 1 found no missing first-owner handoff or missing 734 receipt that blocks
797 from continuing.

## Suggested Next

Execute Step 2 from `plan.md`: compare dispatcher, importer destination
containers, and reachable verifier behavior against the terminal matrix.
Repair only matrix-proven gaps for rows that already have accepted typed
receiver or evidence-backed no-change dispositions.

## Watchouts

- 813's closed 848/849/850 dispositions are evidence/no-change inputs and do
  not fabricate direct 734 receiver work.
- 847 is terminal deletion evidence for 797 only; it does not prove 797
  complete.
- Preserve open 795, 796, 821, and 822 scopes. Do not absorb body-parameter,
  residual instruction/terminator, inline-assembly, or switch-selector routes
  into this 797 packet.
- Do not reopen accepted 734 Step 7.52 or the closed 867 producer handoff.
- Reject text, `monostate`, rendered operand, printer output, testcase
  identity, or compatibility-mirror authority.

## Proof

Documentation-only proof:

```sh
find docs/lir_to_new_bir_final_coverage_convergence -maxdepth 1 -type f -printf '%f\n' | sort
git diff --check
```

`test_after.log` was not produced because this delegated packet's proof is a
documentation inventory plus whitespace check, not a build/test command.
