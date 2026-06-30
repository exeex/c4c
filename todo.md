Status: Active
Source Idea Path: ideas/open/449_pointer_relational_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Relational Branch Packet

# Current Packet

## Just Finished

Completed Step 3: implemented the first unsigned pointer relational branch
publication and RV64 consumption packet.

Code/test changes:

- Extended `plan_prepared_fused_pointer_branch_publication` to accept unsigned
  pointer relational predicates `Ult/Ule/Ugt/Uge` in addition to the closed
  pointer `Eq/Ne` route.
- Extended the RV64 prepared pointer branch consumer to lower:
  - `Ult` as `bltu`;
  - `Uge` as `bgeu`;
  - `Ugt` as swapped `bltu`;
  - `Ule` as swapped `bgeu`.
- Kept raw pointer branch normalization closed; only prepared branch-condition
  authority opens this route.
- Added BIR contract coverage for unsigned pointer relational publication and
  signed relational rejection.
- Added RV64 object coverage for all four unsigned pointer relational
  predicates and preserved fail-closed signed relational, non-null immediate,
  missing-branch-condition, missing-home, and target-mismatch cases.

Representative evidence:

| Row | Result |
| --- | --- |
| `20010329-1 main.entry` prepared `branch_condition entry kind=fused_compare condition=%t8 compare=uge ptr %t5, %t7` with `%t5`, `%t7`, `%t8` GPR homes | RV64 object probe no longer stops at `unsupported_terminator_fragment`; it now reaches `unsupported_move_bundle_target_shape`, outside this packet. |
| signed pointer relational predicates | remain rejected |
| non-null pointer immediates / missing branch condition / missing operand home / target mismatch | remain rejected |

Artifacts:

- `build/agent_state/449_step3_unsigned_pointer_relational_branch/summary.md`
- `build/agent_state/449_step3_unsigned_pointer_relational_branch/20010329-1.prepared.out`
- `build/agent_state/449_step3_unsigned_pointer_relational_branch/20010329-1.object.err`

## Suggested Next

Execute Step 4: residual disposition and close-readiness review for idea 449.
Suggested owned files:

- `todo.md`
- `test_after.log`
- `build/agent_state/449_step4_residual_disposition/*`

Step 4 should re-probe representative pointer relational rows, confirm
`20010329-1 main.entry` no longer owns `unsupported_terminator_fragment`, and
classify remaining failures as pointer relational follow-up, select-result
publication, stack-home/move-bundle materialization, pointer-value/provenance,
or other first owners.

Exact proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Keep this plan limited to pointer relational fused branch predicates.
- Do not reopen pointer `eq/ne` branch publication; closed idea 441 owns that
  route.
- Do not fold select-result branch publication into this plan; that belongs to
  `ideas/open/450_select_result_branch_publication.md`.
- Do not infer pointer relational semantics from raw BIR compare/branch shape,
  block order, filenames, function names, testcase names, or one dump layout.
- Keep missing, incoherent, unsupported, or policy-ineligible relational branch
  authority fail-closed.
- Do not widen the completed unsigned relational branch route into
  select-result publication, stack-home/move-bundle materialization, or
  pointer-value/provenance publication without a new delegated packet.
- Keep `930930-1` stack-home and pointer-value-memory rows out of the first
  consumer packet.
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
