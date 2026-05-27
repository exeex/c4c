# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish variadic aggregate va_arg cursor baseline

## Just Finished

Step 1 established the calls-route baseline for
`c_testsuite_aarch64_backend_src_00204_c`: the delegated focused proof is
still 5/6, with only `00204` failing by `[RUNTIME_NONZERO]` segmentation
fault.

The first `stdarg` call is
`myprintf("%9s %9s %9s %9s %9s %9s", s9, s9, s9, s9, s9, s9)`. Caller-side
argument placement still looks coherent: format in `x0`, the first three
`struct s9` byval arguments in GPR register-save lanes `x1/x2`, `x3/x4`,
`x5/x6`, and the remaining three in outgoing stack slots.

For the expanded `%9s` `va_arg` path in `myprintf`, BIR/prepared BIR show the
intended small-aggregate access model:

- Source va_list home: `%lv.ap` is held in `x21`; va_list object home is the
  stack slot at `sp+1264`.
- Register-save fields: `gp_register_save_area@8`, `fp_register_save_area@16`,
  `gp_offset@24`, `fp_offset@28`; `va_start` initializes
  `overflow_arg_area@0 = sp+1344`, `gp_register_save_area@8 = sp+1128`,
  `gp_offset@24 = -56`.
- `%9s` source class: GPR register-save area while `gp_offset + 16 <= 0`,
  overflow area otherwise.
- Progression field: `gp_offset@24`, stride `16` for register-save
  consumption.
- Overflow source/progression field: `overflow_arg_area@0`, stride `16`.
- Register-save lane homes: two 8-byte GPR lanes per `struct s9`; first
  `%9s` reads `sp+1072` and `sp+1080` via `gp_register_save_area + -56`.

The bad cursor is introduced by lowering/emission for the expanded
small-aggregate overflow path, not by caller placement and not by the prepared
HFA `llvm.va_arg.aggregate` access-plan construction. BIR for the `%9s`
overflow block is correct:

- `%t47 = bir.load_local ptr %lv.ap.0`
- `%t48 = bir.add ptr %t47, 16`
- `bir.store_local %lv.ap.0, ptr %t48`
- join phi `%t49` uses the original `%t47` as the copy source.

Prepared BIR preserves that ownership: `%t47` is a register-homed value in
`x13`, `%t48` is the update value, and prepared memory access maps both
`vaarg.stack.36` load/store operations to the `overflow_arg_area` field slot.
The emitted AArch64 for `.LBB154_25`, however, computes the update from
`ldr x9, [sp, #24]` instead of from the loaded `%t47`/`x13`, stores that value
back to `[x21]`, reloads it into `x13`, and then the join copy dereferences
`x13`. At the crash this produces `x13 = 0x10` before `ldrb w9, [x13]`.

Relevant dumps were inspected in `/tmp/00204.bir.txt`,
`/tmp/00204.prepared-bir.txt`, and `/tmp/00204.s`. `--dump-mir` currently
reports only the contract-first debug surface, so the useful machine evidence
is the emitted assembly.

## Suggested Next

Repair the expanded small-aggregate `va_arg` overflow path so the
`overflow_arg_area` update is computed from the authoritative loaded cursor
value (`%t47`/`x13`) and the join copy keeps using that original cursor as its
source. The bounded code packet should inspect the prepared-memory/dispatch
lowering for `vaarg.stack.36` `%t47 -> %t48 -> store %lv.ap.0` rather than the
HFA-only `make_variadic_aggregate_va_arg_record` path, then add/use focused
same-feature probes for register-save and overflow small aggregate `va_arg`.

## Watchouts

The HFA `llvm.va_arg.aggregate` helper operands have prepared access-plan
fields and print through `print_aggregate_va_arg_lowering_lines`; those are
nearby guardrails but they are not the direct source of the `%9s` crash. The
direct source is the already-expanded BIR small-aggregate `vaarg.stack.36`
path.

Keep the ALU 5/6 focused subset as a guardrail. The ALU idea remains open.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: 5/6 passed. Passing tests were the two ALU prepared-authority probes,
`00164`, `00176`, and `00181`; failing test was
`c_testsuite_aarch64_backend_src_00204_c` with segmentation fault.

Proof log: `test_after.log`.
