Status: Active
Source Idea Path: ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Enable Public `return_add.c` Smoke

# Current Packet

## Just Finished

Step 5 semantic precondition completed for `plan.md` Step 5, `Enable Public return_add.c Smoke`, without wiring the public smoke.

Implemented and tightened a general returned scalar add/sub route for prepared results that are rematerialized as immediates:
- When source BIR returns a named add/sub result but prepared storage rematerializes that result as an immediate, the AArch64 module builder can now use the prepared before-return ABI move as the selected scalar result register.
- Preserved structured scalar operands, including immediate operands, rather than matching `return_add.c` by name or rewriting expectations.
- Extended the machine printer to emit selected scalar add/sub with register and immediate operands.
- The selected scalar printer now accepts immediate operands only when they fit the plain unshifted AArch64 `#imm` form currently emitted: signed integer `0..4095`.
- Negative immediates and out-of-range immediates fail closed before assembly text is produced.
- Added focused prepared-module and machine-printer tests for the rematerialized returned-add shape, accepted `#3`, rejected `4096`, and rejected `-1`.
- The public `return_add.c` semantic precondition route still emits `mov w0, #2`, structured `add w0, w0, #3`, and `ret`.

Kept out of scope as required: no public smoke enablement, no backend CMake wiring changes, no edits to `tests/backend/case/return_add.c`, `plan.md`, or the source idea.

## Suggested Next

Delegate the next coherent packet to wire the public `tests/backend/case/return_add.c` smoke if the supervisor accepts this semantic precondition and wants CMake enablement.

## Watchouts

- Printer coverage now exists for selected scalar add/sub with register-register and plain-immediate operand forms plus register-valued return nodes, but do not enable `tests/backend/case/return_add.c` unless the supervisor explicitly delegates public smoke wiring.
- Reject named-case matching, fixture assembly text, expectation downgrades, or unsupported marking as progress.
- Keep `return_add_sub_chain.c` and broader scalar ALU cases deferred unless they receive matching structured proof.
- The current printer supports register-register, register-immediate, commuted add immediate-register, and immediate-immediate scalar add/sub only for signed integer immediates in `0..4095`. Sub with immediate lhs and register rhs remains fail-closed.
- Watch for `I32` versus `I64` register views (`w` versus `x`) when expanding beyond the covered scalar register-register subset.

## Proof

Ran the supervisor-delegated Step 5 follow-up proof exactly:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "backend_aarch64_(prepared_module_identity|machine_printer|target_instruction_records)$"' > test_after.log 2>&1
```

Result: passed; build plus 3 focused CTest tests passed for `backend_aarch64_(prepared_module_identity|machine_printer|target_instruction_records)$`. Proof log: `test_after.log`.

Done-when route check:

```sh
build/c4cll --codegen asm --target aarch64-linux-gnu tests/backend/case/return_add.c -o /tmp/c4c_return_add.s
```

Result: passed; generated assembly contains `add w0, w0, #3` and `ret`.

Supervisor-side broader validation now preserved in `test_after.log`:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_aarch64_"' > test_after.log 2>&1
```

Result: passed; 24/24 `backend_aarch64_` tests passed.
