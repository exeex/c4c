Status: Active
Source Idea Path: ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh outgoing-stack argument evidence

# Current Packet

## Just Finished

Completed Step 1: refreshed outgoing stack-slot, byval, and aggregate
stack-copy call argument evidence before implementation.

Artifacts are under
`build/agent_state/624_step1_outgoing_stack_refresh/`:

- `README.md`: concise command, row, bucket, and guard summary.
- `command_status.tsv`: per-row return codes for semantic BIR, prepared BIR,
  RV64 asm, and RV64 object runner.
- `call_arg_evidence.tsv`: extracted prepared callsite and argument-placement
  rows.
- `dumps/<row>/dump-bir.txt`: semantic BIR or first semantic-lowering
  diagnostic.
- `dumps/<row>/dump-prepared-bir.txt`: prepared BIR or first
  semantic-lowering diagnostic.
- `asm/<row>/out.s` and `asm/<row>/codegen-asm.log`: RV64 asm evidence.
- `rv64_object/<row>/case.log`: RV64 object runner diagnostics.

Refresh commands used:

- `./build/c4cll -I tests/c/external/gcc_torture --dump-bir --target riscv64-linux-gnu <row>`
- `./build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu <row>`
- `./build/c4cll -I tests/c/external/gcc_torture --codegen asm --target riscv64-linux-gnu <row> -o <artifact>/asm/<row>/out.s`
- `cmake -P tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`
  with all outputs rooted under the Step 1 artifact directory.

Rows refreshed:

- `src/20000808-1.c`
- `src/931004-1.c` through `src/931004-14.c`
- `src/931031-1.c`
- `src/950607-2.c`
- `src/pr69447.c`

First-owner buckets:

- Prepared call-boundary producer missing aggregate destination authority:
  `src/20000808-1.c`, odd `src/931004-*` rows
  (`1,3,5,7,9,11,13`), `src/931031-1.c`, and
  `src/950607-2.c`. These rows reach prepared BIR and expose same-module
  aggregate/byval call arguments, but the aggregate args report
  `bank=aggregate_address ... to=none`.
- Semantic local-memory guard rows: even `src/931004-*` rows
  (`2,4,6,8,10,12,14`). These stop before prepared handoff with
  `semantic lir_to_bir function 'f' failed in load local-memory semantic
  family`.
- Scalar outgoing-stack guard row: `src/pr69447.c`. It is not aggregate/byval
  producer proof; it shows the scalar overflow path already has explicit
  destination authority with `arg8 bank=gpr from=immediate:1 to=stack+0:size=8`.

Representative negative guard evidence:

- `src/20000808-1.c`: same-module `foo -> f` call has six byval aggregate
  args; prepared `arg0` through `arg5` are `bank=aggregate_address` and
  `to=none`; RV64 object diagnostic is `unsupported_call_abi`.
- Odd `src/931004-*` rows: same-module `main -> f` call has three byval
  aggregate args; the third source may already be a local frame slot, but its
  outgoing destination remains `to=none`; RV64 object diagnostic is
  `unsupported_call_abi`.
- `src/931031-1.c`: one byval aggregate arg remains `to=none`.
- `src/950607-2.c`: three byval aggregate args remain `to=none`.
- `src/pr69447.c`: scalar stack arg destination exists and should remain a
  guard against regressing ordinary scalar outgoing stack facts.

## Suggested Next

Execute Step 2 from `plan.md`: locate the prepared call-boundary producer
surface that should publish aggregate/byval outgoing destination stack offset
and size facts. Start from the prepared call-plan rows where aggregate args are
`to=none`, and identify the producer data structure that already knows the
source payload identity, callsite, argument index, outgoing area, and size.

## Watchouts

- Do not infer aggregate destination offsets inside RV64 from ABI index, final
  assembly layout, source filename, or testcase shape; current prepared
  evidence explicitly shows missing aggregate destination authority.
- Keep `src/pr69447.c` as a scalar outgoing-stack negative/guard row, not as
  proof that aggregate/byval destination facts exist.
- Keep even `src/931004-*` rows out of the first implementation proof until
  their semantic local-memory blocker is separately owned.
- Do not broaden into variadic/library policy, runtime mismatch, local/global
  producers, stack-frame consumers, expectations, unsupported markers,
  allowlists, timeouts, or accounting changes.

## Proof

Proof command:
`cmake --build build --target c4cll > test_after.log 2>&1`

Result: passed. `test_after.log` contains `ninja: no work to do.` after the
target check.
