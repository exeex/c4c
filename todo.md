# Current Packet

Status: Active
Source Idea Path: ideas/open/221_aarch64_markdown_driven_backend_case_bringup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Commit The Bring-Up Matrix

## Just Finished

Completed plan Step 2, `Commit The Bring-Up Matrix`, as a docs-only matrix
packet.

Added `src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md` with durable
columns for markdown owner, structured owner `.cpp/.hpp`, dependent
machine-node/operator coverage, case files, external smoke expectation, status,
and deferred/blocked reason.

The matrix records the current enabled cases honestly:

- `aarch64_return_zero_smoke.c` remains the positive public AArch64 smoke with
  external assembler/compiler proof.
- `aarch64_no_selected_machine_nodes.c` remains the negative machine-node guard
  that must fail closed before producing misleading output.

The matrix records deferred or blocked groups for returns, ALU, comparison,
memory, bulk memory builtins, calls/prologue, aggregate and variadic ABI cases,
globals, inline asm, FP/cast/i128/atomic/intrinsic families, and the reasons
each group cannot be enabled yet.

The matrix includes explicit testcase-overfit and expectation downgrade reject
rules. It identifies scalar return/ALU as the first next feature group with the
route `alu.md` plus `returns.md` -> `records.cpp/.hpp` plus
`machine_printer.cpp` and selection owner -> `return_add.c`.

## Suggested Next

Execute plan Step 3, `Validate Existing Public Smoke Route`: run the narrow
currently enabled backend case smoke proof and record whether it uses public
`c4cll --codegen asm --target aarch64-linux-gnu` output plus external
assembler/compiler validation.

## Watchouts

- Do not touch `ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md`.
- Do not enable cases before the corresponding markdown feature has a structured implementation route.
- Do not weaken expectations or add testcase-shaped backend shortcuts.
- Keep public smoke validation on the c4cll route with external assembler/compiler proof where applicable.
- `run_backend_aarch64_external_smoke.cmake` compiles the emitted assembly with
  `clang` and assembles with `aarch64-linux-gnu-as`; it does not use the
  internal assembler, encoder, object writer, linker, or binary output route.
- The first next implementation candidate named by the matrix is scalar
  return/ALU, but Step 3 should validate the existing route before any case
  enablement work.

## Proof

Delegated matrix proof:

```bash
bash -lc 'set -o pipefail; rg -n "markdown owner|structured owner|machine-node|smoke expectation|enabled|deferred|blocked|returns.md|alu.md|comparison.md|memory.md|calls.md|prologue.md|globals.md|testcase-overfit|expectation downgrade|aarch64_return_zero_smoke|aarch64_no_selected_machine_nodes|return_add" src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md todo.md > test_after.log 2>&1; test -s test_after.log'
```

Proof log path: `test_after.log`. This is sufficient for Step 2 because the
packet is docs/matrix authoring only and did not touch implementation or test
wiring.
