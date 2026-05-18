Status: Active
Source Idea Path: ideas/open/278_aarch64_c_testsuite_undefined_temporary_labels.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Label Ownership

# Current Packet

## Just Finished

Step 2 from `plan.md` traced the representative undefined temporary-label
family through prepared control flow, AArch64 branch lowering, and terminal
assembly printing.

Ownership trace:

- Prepared BIR/control-flow has the required source facts. For representative
  `00005.c`, `--dump-prepared-bir` shows blocks `entry`, `block_1`, `block_2`,
  `block_3`, `block_4`, `block_5`, `block_6`, `block_7`, and `block_8`, with
  branch facts such as `block_4 terminator=branch target=block_5` and
  conditional true/false targets for `entry`, `block_2`, and `block_5`.
- `src/backend/mir/aarch64/codegen/traversal.cpp` transfers every
  `PreparedControlFlowBlock::block_label` into `MachineBlock::block_label`
  before dispatching block contents.
- `src/backend/mir/aarch64/codegen/comparison.cpp` validates branch terminator
  target ids against retained BIR terminators and records
  `BranchTargetOperand{function_name, block_label}` for unconditional,
  materialized-bool, and fused-compare branches.
- `src/backend/mir/aarch64/codegen/instruction.cpp` preserves those branch
  target operands as selected branch machine-node operands.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp` renders branch targets
  as `.LBB<function_id>_<block_label>`, for example `.LBB1_6`.
- `src/backend/mir/printer.hpp` prints only target instruction lines for each
  `MachineBlock`; it does not emit a block label definition before a block's
  instruction stream.
- `src/backend/mir/aarch64/codegen/asm_emitter.cpp` emits only the public
  function label (`main:`) before delegating each function body to the common
  MIR printer.

Conclusion: this is backend printer/emitter output ownership, not frontend,
common block labeling, prepared handoff, or AArch64 branch target
materialization. The missing definitions should be emitted from the common MIR
function printer using the already-present `MachineFunction::function_name` and
`MachineBlock::block_label` facts, or through an equivalent AArch64 assembly
emitter hook if the common printer contract is deliberately kept
instruction-only. The implementation should define every non-entry referenced
block label in the function body, using the same `.LBB<function>_<block>`
spelling as AArch64 branch operands, without matching c-testsuite filenames or
exact generated label numbers.

Observed generated assembly still contains references with no definitions, for
example `00005.c.s` has `b .LBB1_6` and no `.LBB1_6:` line; `00006.c.s` has
two `b .LBB1_2` references and no `.LBB1_2:` line; `00220.c.s` has two
`b .LBB164_2` references and no `.LBB164_2:` line.

## Suggested Next

Run Step 3: add focused backend proof for MIR block-label emission before the
implementation packet. Likely owned files are:

- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp` or
  `tests/backend/mir/backend_aarch64_mir_carrier_test.cpp` for a focused old
  path proof that branch target references require matching block-label
  definitions.
- `src/backend/mir/printer.hpp` for the likely general implementation in Step
  4, unless supervisor chooses to keep the common printer instruction-only and
  instead implement an AArch64-specific wrapper in
  `src/backend/mir/aarch64/codegen/asm_emitter.cpp`.

The test should model a multi-block `MachineFunction` with a branch to another
block and assert that printed assembly includes both the branch reference and
the matching `.LBB<function>_<block>:` definition.

## Watchouts

- Do not re-activate the parked runtime-runner route from
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md` unless an
  AArch64 host or `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Do not match c-testsuite filenames, exact case names, or exact `.LBB`
  spellings instead of repairing label authority or emission semantics.
- The first implementation packet should use the same representative subset as
  its narrow c-testsuite proof:
  `ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00005|00006|00040|00156|00220)_c$'`.
- The current failing stage is `[BACKEND_FAIL]`; keep assembler/link,
  backend-printer, frontend, `lir_to_bir`, and runtime classifications
  stage-specific if later packets uncover them.
- Existing printer tests such as
  `selected_branch_and_store_nodes_print_without_semantic_roundtrip` currently
  assert instruction-only output. If Step 4 changes the common MIR printer,
  update focused expectations semantically to require label definitions, not by
  weakening the branch checks.
- Entry block definitions need care: `asm_emitter.cpp` already emits the public
  function label before the function body. The next packet should decide
  whether the common printer emits labels for all blocks, only non-entry
  blocks, or all blocks except those whose public function label already names
  the same address.

## Proof

Delegated proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00005|00006|00040|00156|00220)_c$'; } 2>&1 | tee test_after.log
```

Result: configure/build succeeded; CTest selected 5 tests, 0 passed, 5 failed.
All selected failures remain `[BACKEND_FAIL]` undefined temporary `.LBB...`
labels:

- `00005.c`: `.LBB1_6`
- `00006.c`: `.LBB1_2`
- `00040.c`: `.LBB99_2`, `.LBB99_7`, `.LBB99_9`, `.LBB99_11`,
  `.LBB99_13`, `.LBB100_16`, `.LBB100_25`
- `00156.c`: `.LBB89_2`
- `00220.c`: `.LBB164_2`

The nonzero proof exit is the expected trace/classification baseline for this
packet. Proof log path: `test_after.log`.
