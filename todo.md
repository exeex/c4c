Status: Active
Source Idea Path: ideas/open/797_lir_to_new_bir_final_coverage_convergence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Align documentation and final proof record

# Current Packet

## Just Finished

Completed plan Step 3 transactional proof packet.

The selected focused transactional/backend proof covered the LIR-to-BIR
interface, selected pointer authority, pipeline identity, execution control,
and checkpoint surfaces. The broader backend proof then passed the full
`^backend_` subset, including the matrix-relevant selected direct-local
`LirVaStartOp` receipt and neighboring backend identity/authority tests.

## Suggested Next

Execute Step 4 from `plan.md`: align the final matrix/proof documentation,
run `git diff --check`, and run the supervisor-selected final broader/full
validation gate for 797 acceptance.

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
- `src/backend/bir/lir_to_bir/README.md` still has older planning rows saying
  generic `va_start` semantic payload wiring is missing; Step 2 did not edit
  that broader planning note because the matrix-authorized row is narrower:
  selected direct-local `VaStartAuthorityNode` receipt only.

## Proof

Focused proof:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$|^backend_lir_selected_pointer_authority$|^backend_bir_pipeline_identity$|^backend_bir_execution_control$|^backend_bir_checkpoint$'; } > test_after.log 2>&1
```

Result: passed.

Broader backend proof:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.
