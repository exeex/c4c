# Current Packet

Status: Active
Source Idea Path: ideas/open/221_aarch64_markdown_driven_backend_case_bringup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Select The First Feature Group

## Just Finished

Completed plan Step 4, `Select The First Feature Group`, as a lifecycle
handoff packet.

The selected first feature group is scalar return/ALU, as named by the
committed matrix in
`src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md`.

Selected implementation route:

```text
src/backend/mir/aarch64/codegen/alu.md
  + src/backend/mir/aarch64/codegen/returns.md
  -> src/backend/mir/aarch64/codegen/records.cpp/.hpp
  -> selected scalar machine-node/operator coverage
  -> src/backend/mir/aarch64/codegen/machine_printer.cpp
  -> tests/backend/case/return_add.c
```

The separate implementation initiative is recorded at:

- `ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md`

## Suggested Next

The supervisor can either:

- Continue Step 5 of the current matrix plan to validate close readiness for
  idea 221.
- Switch lifecycle state to the new scalar return/ALU implementation idea when
  ready to begin code work.

Implementation proof ladder for the new idea:

- Start with focused C++ record/model coverage for `ScalarInstructionRecord`,
  `ScalarAluOperationKind`, return payload records, natural mnemonic spelling,
  and return node behavior.
- Add selected scalar machine-node/operator coverage for simple integer add/sub
  results that flow into the returned value.
- Extend `machine_printer.cpp` coverage so printable selected nodes emit real
  AArch64 add/sub spelling and `ret`.
- Only then enable or add the public `tests/backend/case/return_add.c` smoke on
  the `c4cll --codegen asm --target aarch64-linux-gnu` route with emitted text
  checks plus external assemble/link/run proof where applicable.

## Watchouts

- This is lifecycle handoff only, not implementation progress.
- Do not add testcase-shaped `return_add` shortcuts, named-case matching, or
  fixture-specific printer text.
- Do not perform an expectation downgrade or weaken existing enabled AArch64
  smoke/failure contracts.
- Do not enable `return_add.c` before structured selected-node/operator/printer
  coverage exists.
- Keep `return_add_sub_chain.c`, two-argument ALU cases, and broader scalar
  fixtures deferred unless the same semantic route naturally supports them with
  proof.

## Proof

Lifecycle handoff text proof:

```bash
rg -n "scalar return/ALU|return_add|alu.md|returns.md|Reviewer Reject Signals|testcase-shaped|expectation downgrade|Current Step ID: 4|Current Step Title: Select The First Feature Group" todo.md ideas/open src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md > test_after.log 2>&1; test -s test_after.log
```

Proof log path: `test_after.log`. This proof is sufficient for Step 4 because
the packet is lifecycle/docs-only and verifies the selected scalar return/ALU
group, route owners, reject signals, `return_add` handoff, and active step
metadata are present.
