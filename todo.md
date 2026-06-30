Status: Active
Source Idea Path: ideas/open/449_pointer_relational_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: re-probed pointer relational representatives after the Step
3 unsigned pointer relational branch consumer and classified remaining owners.

Residual table:

| Row | Current result | Classification |
| --- | --- | --- |
| `20010329-1` | `unsupported_move_bundle_target_shape` | The former `main.entry` `unsupported_terminator_fragment` from `compare=uge ptr %t5, %t7` is resolved. Current first owner is prepared move-bundle/select-result continuation materialization, outside idea 449. |
| `930930-1` | `unsupported_instruction_fragment` | Fails before a clean terminator consumer. Relational branch rows have stack-slot operand/condition homes and pointer-value/provenance gaps; not an in-scope remaining pointer relational branch packet. |

Closed by this idea:

- Prepared fused pointer relational branches with `Ult/Ule/Ugt/Uge`, matching
  labels, GPR-compatible condition home, and GPR-compatible named operand
  homes.
- `20010329-1 main.entry`
  `branch_condition entry kind=fused_compare condition=%t8 compare=uge ptr %t5, %t7`
  no longer owns `unsupported_terminator_fragment`.

Remaining first owners outside this idea:

- select-result branch publication / move-bundle target materialization for
  `20010329-1` `root_is_select=yes` rows;
- stack-home branch operand/condition materialization for `930930-1` if a later
  plan chooses stack homes in branch consumers;
- pointer-value/provenance publication before considering `930930-1` pointer
  relational rows.

Artifacts:

- `build/agent_state/449_step4_residual_disposition/classification.md`
- `build/agent_state/449_step4_residual_disposition/20010329-1.prepared.out`
- `build/agent_state/449_step4_residual_disposition/20010329-1.object.err`
- `build/agent_state/449_step4_residual_disposition/930930-1.prepared.out`
- `build/agent_state/449_step4_residual_disposition/930930-1.object.err`

## Suggested Next

Plan-owner close-readiness review. Recommendation: close idea 449 for the
selected unsigned pointer relational branch route. Keep active only if the
supervisor identifies a new exact pointer relational branch packet; current
known residuals route to select-result/move-bundle, stack-home, or
pointer-value/provenance owners.

## Watchouts

- Keep this plan limited to pointer relational fused branch predicates.
- Do not reopen pointer `eq/ne` branch publication; closed idea 441 owns that
  route.
- Do not fold select-result branch publication into this plan; route it to
  `ideas/open/450_select_result_branch_publication.md` or another delegated
  owner.
- Do not widen the completed unsigned relational branch route into stack-home
  expansion, move-bundle materialization, or pointer-value/provenance
  publication.
- Keep `930930-1` stack-home and pointer-value-memory rows out of this closed
  route.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Activation-only validation:

```sh
git diff --check
```

Step 1 classification validation:

```sh
git diff --check
```

Result: passed.

Step 2 policy validation:

```sh
git diff --check
```

Result: passed.

Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` contains the backend subset proof. Focused
pre-proof
`cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^(backend_prepare_stack_layout|backend_riscv_object_emission)$'`
passed.

Step 4 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` contains the backend subset proof.
