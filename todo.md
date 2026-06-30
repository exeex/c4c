Status: Active
Source Idea Path: ideas/open/441_terminator_select_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Publication Packet

# Current Packet

## Just Finished

Completed Step 3: implemented the first fused pointer `eq/ne` branch
publication packet.

Code/test changes:

- Added `plan_prepared_fused_pointer_branch_publication` and
  `prepared_fused_pointer_branch_publication_available` as the producer-owned
  predicate for prepared fused pointer branch publication.
- Added RV64 object-route consumption for prepared fused pointer `eq/ne`
  branches only when the prepared branch condition, terminator condition,
  labels, condition home, and operand homes are coherent.
- Kept raw pointer compare fallback closed; pointer relational predicates and
  select-result publication were not included.
- Added focused BIR contract coverage for accepted fused pointer branch
  publication and fail-closed missing/incoherent/unsupported shapes.
- Added focused RV64 object coverage for prepared pointer `ne` and `eq`
  register-register branches plus existing pointer-null `ne`, while preserving
  fail-closed relational, missing branch-condition, missing-home,
  target-mismatch, and non-null pointer immediate cases.

Representative evidence:

| Row | Result |
| --- | --- |
| `20041112-1 bar.entry` prepared `branch_condition entry kind=fused_compare condition=%t6 compare=ne ptr %t2, %t5` with `%t2`, `%t5`, `%t6` GPR homes | RV64 object probe succeeds and writes `build/agent_state/441_step3_fused_pointer_branch_publication/20041112-1.o` |
| pointer relational predicates such as `uge ptr` | remain rejected |
| missing prepared branch condition / missing operand home / target mismatch / non-null pointer immediate | remain rejected |

Artifacts:

- `build/agent_state/441_step3_fused_pointer_branch_publication/summary.md`
- `build/agent_state/441_step3_fused_pointer_branch_publication/20041112-1.prepared.out`
- `build/agent_state/441_step3_fused_pointer_branch_publication/20041112-1.object.err`

## Suggested Next

Execute Step 4: residual disposition and close-readiness review for idea 441.
Suggested owned files:

- `todo.md`
- `test_after.log`
- `build/agent_state/441_step4_residual_disposition/*`

Step 4 should re-probe representative terminator/select rows, confirm
`20041112-1 bar.entry` no longer stops at fused pointer branch publication,
classify any remaining direct-global select-chain or generic select/terminator
residuals by first owner, and decide whether idea 441 is close-ready or needs a
new bounded packet.

Exact proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Keep this plan limited to terminator/select publication authority.
- Do not reopen direct-global return authority; closed idea 440 routes only the
  remaining `bar.entry` fused pointer compare branch here.
- Do not infer branch conditions, select results, compare operands, or
  terminator homes from raw BIR shape, block order, filenames, function names,
  or one dump layout.
- Keep any follow-up separate from this completed fused pointer `eq/ne` packet.
- Do not include pointer relational predicates (`ult/uge/ule`) or select-chain
  materialization unless the supervisor delegates a new packet.
- Keep missing or incoherent publication authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log` or `review/`.

## Proof

Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` contains the backend subset proof. Focused
pre-proof
`cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^(backend_prepare_stack_layout|backend_riscv_object_emission)$'`
passed.
