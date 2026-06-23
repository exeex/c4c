Status: Active
Source Idea Path: ideas/open/311_rv64_ordinary_control_expression_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Control/Expression Coverage

# Current Packet

## Just Finished

Step 1: Normalize Representative Failure Evidence is complete. Matched RV64
`--dump-bir`, `--dump-prepared-bir --mir-focus-function main`, and
`--codegen asm` artifacts were generated under
`build/rv64_c_testsuite_probe_latest/triage_311_step1/` for
`src/00008.c`, `src/00030.c`, and `src/00105.c`; all commands exited `0`.

Step 4 classification already placed all three in
`incomplete_control_or_expression_emission`. The normalized evidence shows the
same first semantic emission boundary across the representatives:

- `src/00008.c`: BIR and prepared BIR contain `dowhile.cond.1` with
  `bir.ne i32 %t2, 0`, `bir.cond_br`, and prepared
  `branch_condition ... kind=fused_compare ... can_fuse_with_branch=yes`;
  emitted RV64 stops at `.Lmain_dowhile_cond_1: lw t0, 0(sp)`.
- `src/00030.c`: BIR and prepared BIR contain repeated `f()` call-result
  comparisons followed by `bir.cond_br`; prepared call plans publish `a0` to
  `t0`, and prepared control flow records six `kind=fused_compare` branch
  conditions; emitted RV64 stops after `call f` and `mv t0, a0`.
- `src/00105.c`: BIR and prepared BIR contain `for.cond.1` with
  `bir.slt i32 %t0, 10`, `bir.cond_br`, and prepared
  `branch_condition ... kind=fused_compare ... can_fuse_with_branch=yes`;
  emitted RV64 stops at `.Lmain_for_cond_1: lw t0, 0(sp)`.

Selected first repair boundary: RV64 emission for prepared `cond_branch`
terminators whose `branch_condition` is `kind=fused_compare`, specifically the
tail after operand publication that must lower the compare and emit the
true/false successor transfer. This covers loop predicates and call-result
predicates without filename matching because the common prepared-control-flow
contract is the same in all three representatives. Return-value finalization,
epilogues, and final `ret` emission are deferred until execution can reach the
successor return blocks; stack-slot/address materialization and external-stub
policy are not part of this boundary.

## Suggested Next

Execute Step 2 from `plan.md`: add focused control/expression coverage for
RV64 prepared fused-compare conditional branches. The test shape should cover
at least one local-loaded loop predicate and one call-result predicate flowing
through a compare into a conditional branch, with semantic assertions on the
emitted compare/branch successor behavior rather than c-testsuite filenames.

## Watchouts

- Do not implement filename-shaped logic or c-testsuite-specific shortcuts.
- Do not downgrade supported-path expectations or mark cases unsupported.
- Keep stack/local address materialization and external-stub policy separate
  unless they directly block ordinary control/expression completion proof.
- The current first boundary is not a generic append-epilogue problem. The
  emitted functions stop before the prepared fused compare/conditional branch;
  appending a `ret` would hide the missing control transfer instead of repairing
  it.

## Proof

Read-only evidence normalization plus targeted artifact generation:

- `./build/c4cll --dump-bir --target riscv64-linux-gnu <case>`
- `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function main <case>`
- `./build/c4cll --codegen asm --target riscv64-linux-gnu <case> -o build/rv64_c_testsuite_probe_latest/triage_311_step1/<stem>.s`

Cases: `tests/c/external/c-testsuite/src/00008.c`,
`tests/c/external/c-testsuite/src/00030.c`, and
`tests/c/external/c-testsuite/src/00105.c`. No build or CTest proof was
required by the delegated packet; no `test_after.log` was produced.
