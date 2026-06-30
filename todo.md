Status: Active
Source Idea Path: ideas/open/450_select_result_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Select-Result Publication Contract

# Current Packet

## Just Finished

Completed Step 2: defined the select-result branch publication and move-bundle
materialization contract for idea 450. Supporting artifact:
`build/agent_state/450_step2_select_result_contract/contract.md`.

Accepted first-packet facts:

| Required fact | Accepted boundary |
| --- | --- |
| Select-result identity | Prepared `select_chain` has `root_is_select=yes` and `source_producer=select_materialization`. |
| Branch condition | Prepared `branch_condition` compares that select-result value against integer zero with `Eq` or `Ne`, has coherent true/false labels, and is fuseable with the branch. |
| Homes | Select-result value and branch-condition value both have GPR-compatible prepared homes. |
| Move-bundle target | The selected result has available `block_entry_publication` / out-of-SSA move-bundle target facts with `destination_kind=value` and `destination_storage=register`. |
| Source materialization | Immediate/register incoming values are consumed only through prepared move-bundle source facts and value ids, not rediscovered from raw BIR. |

Rejected adjacent shapes include raw-only select branches, missing
`select_chain`, missing `branch_condition`, non-zero compare constants,
predicates outside `Eq`/`Ne`, missing or non-GPR homes, unavailable or
non-register publication targets, current instruction-side-blocked
`20000622-1`, and stack-home `930930-1` rows that belong to idea 451.

## Suggested Next

Step 3 should implement `Consume Register-Home Select-Result Branch
Publications`: consume the accepted prepared contract in the RV64 object route
for the `20010329-1` `%t22/%t36/%t50` shape, with focused accepted and
fail-closed coverage.

Suggested Step 3 owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/450_step3_select_result_branch_publication/*`

Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Keep this plan limited to select-result branch publication and move-bundle
  target materialization.
- Do not reopen pointer `eq/ne` or unsigned relational branch publication;
  those routes are closed by ideas 441 and 449.
- Do not fold stack-home branch operand materialization into this plan; that
  belongs to `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep `20000622-1` out of the first implementation packet while its current
  first object-route owner is `unsupported_instruction_fragment`.
- Keep `930930-1` stack-slot select-result publication out of this packet;
  its current prepared rows are not register-destination consumer facts.
- Do not accept `root_is_select=yes` alone as authority; Step 3 must require
  the full prepared select-chain, branch-condition, homes, and publication
  target contract.
- Do not consume non-zero compare constants or predicates outside `Eq`/`Ne`
  in the first packet.
- Do not infer select-result publication from raw select shape,
  `root_is_select=yes`, raw `ne i32 <select>, 0`, block order, filenames,
  function names, or one dump layout.
- Keep missing or incoherent select-result publication authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 contract-only validation:

```sh
git diff --check
```

Result: passed.
