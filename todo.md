Status: Active
Source Idea Path: ideas/open/659_rv64_byval_prepared_call_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Focused Byval Evidence

# Current Packet

## Just Finished

Completed `plan.md` Step 1: refreshed focused evidence for the nine
RV64 byval/prepared call-boundary rows under the delegated proof command.

Selected first owner for Step 2: RV64 route/codegen consumption of prepared
`before_call` call-argument ABI moves, especially stack-slot aggregate
transport and outgoing stack argument area emission.

Concrete row evidence:

- `backend_dump_riscv64_byval_preserved_pointer_args` and
  `backend_dump_riscv64_byval_aggregate_fixed_call` are red because expected
  dump snippets drifted, but the actual prepared dump already publishes
  `arg.aggregate_transport=stack_copy`, `outgoing_stack_argument_area=16`,
  `destination_kind=call_argument_abi destination_storage=stack_slot`, and
  explicit call-argument freshness/publication facts.
- `backend_codegen_route_riscv64_byval_aggregate_fixed_call` and
  `backend_codegen_route_riscv64_byval_preserved_pointer_args` fail missing
  `addi sp, sp, -16`; generated caller assembly reaches `main` setup but does
  not emit the outgoing stack-argument area/call sequence for the byval
  aggregate stack argument.
- `backend_codegen_route_riscv64_byval_formal_gpr_publication` fails missing
  `li a2, 5`; callee entry consumes `a2`, but generated caller assembly stops
  before publishing the formal GPR argument.
- Runtime rows 207, 208, and 209 all fail with
  `[BACKEND_RV64_QEMU_UNEXPECTED_RETURN] exit=Illegal instruction expected=0`,
  consistent with incomplete/invalid caller-side route emission after the
  prepared call-boundary facts are present.

Smaller split: keep
`backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload` separate
from the Step 2 byval call-boundary implementation packet. It fails before the
same proof surface with `[BACKEND_RV64_OBJ_EMIT_FAIL]` and
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering; function=main; block=entry; instruction_index=7;
instruction_kind=BinaryInst; owner=i32 %t2`.

## Suggested Next

Executor should run Step 2 against the selected owner: repair RV64 call-route
emission so it consumes the prepared `before_call` `call_argument_abi` moves,
allocates/emits the 16-byte outgoing stack argument area for byval aggregate
stack copies, copies aggregate payload chunks to that area, publishes scalar
GPR call arguments such as `a2`, and then emits the call/restore sequence.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, runtime policy,
  timeout settings, or baseline acceptance files.
- Do not merge pointer-local, stack fan-in, AArch64, CLI, static object-data,
  callee-saved GPR, packed-member, or LLVM torture work into this route.
- Reject named-case or final-assembly-shape fixes.
- Do not treat the two dump failures as permission to rewrite expectations;
  their current output is useful positive evidence that prepared facts exist.
- Keep the object-runtime `BinaryInst` unsupported-fragment row as a separate
  split unless the supervisor explicitly assigns object-route coverage.

## Proof

Ran exactly:

```sh
cmake --build --preset default && (ctest --test-dir build -j --output-on-failure -R 'backend_(dump_riscv64_byval_aggregate_fixed_call|codegen_route_riscv64_byval_aggregate_fixed_call|dump_riscv64_byval_preserved_pointer_args|codegen_route_riscv64_byval_preserved_pointer_args|codegen_route_riscv64_byval_formal_gpr_publication|rv64_runtime_riscv64_byval_aggregate_fixed_call|rv64_runtime_riscv64_byval_preserved_pointer_args|rv64_runtime_riscv64_byval_formal_gpr_publication|obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload)' > test_after.log; test -s test_after.log)
```

Result: build was up to date, CTest captured focused red evidence for all
nine delegated rows, and `test_after.log` is populated.
