Status: Active
Source Idea Path: ideas/open/330_aarch64_non_hfa_aggregate_va_arg_materialization.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Localize Missing Ordinary Dispatch Of Prepared Va Arg Memory Operations

# Current Packet

## Just Finished

Step 2.1 localized the ordinary AArch64 dispatch break without implementation
edits. The prepared BIR for `myprintf` contains the per-byte `LoadLocalInst` /
`StoreLocalInst` copy sequence in the retained ordinary blocks before the
observing calls:

- `vaarg.join.14` has prepared accesses at inst 1-28 and the observing
  `printf(@.str31, %lv.t7.0)` call/address materialization at inst 29.
- `vaarg.join.39` has prepared accesses at inst 1-36 and the observing
  `printf(@.str33, %lv.t9.0)` call/address materialization at inst 37.

Those records are attached to the join blocks, matched by exact
`block_label`/`inst_index`, and are scheduled before the calls. They also have
source addresses: the first copy stage loads from pointer-value bases `%t24`
and `%t49`, while call setup materializes `%lv.t7.0` at stack offset 8 and
`%lv.t9.0` at stack offset 15.

The concrete break is in the ordinary memory-lowering path reached from
`dispatch_prepared_block`. `dispatch_prepared_block` visits ordinary BIR
instructions in block order and calls `lower_memory_instruction` for the byte
loads/stores. For the initial pointer-value byte loads
`%t25.*.memcpy.copy.* = bir.load_local i8 ..., addr %t24` and
`%t50.*.memcpy.copy.* = bir.load_local i8 ..., addr %t49`, prepared addressing
is present and the pointer source homes are register-backed, but the load
results are stack-slot/frame-slot backed. `make_load_memory_instruction_record`
currently accepts only register result homes and register result storage, so
these prepared loads fail as handled-with-no-instruction before selection.
Dispatch then continues to the later address materialization and call emission,
leaving no copy instructions before `.str31`/`.str33`.

Classification: not unattached, not attached to skipped/replaced blocks, not
missing address operands, and not scheduled after the observing call. The first
bad fact is filtering by load-result home/storage kind for stack-slot-result
pointer loads inside ordinary memory lowering. The later frame-slot aggregate
copy records are present too, but they depend on the first-stage pointer loads
materializing the `%t25.*` / `%t50.*` temp bytes.

## Suggested Next

Execute Step 2.2 as a narrow ordinary-dispatch repair: teach the prepared
memory load path how to materialize pointer-value loads whose BIR result home is
a stack slot, for example by loading through a scratch/register result and
publishing/storing that byte to the prepared stack-slot result in the original
BIR order.

After that repair, re-check `vaarg.join.14` and `vaarg.join.39` before the
observing calls. If the later `%lv.t7.aggregate.copy.*` /
`%lv.t9.aggregate.copy.*` frame-slot copy records still do not emit, localize
that as a separate ordinary memory-lowering suppression rather than widening
call-boundary replay.

## Watchouts

Avoid replaying all prepared memory accesses at the call boundary. The missing
records are already in the ordinary block before the call; the break is the
ordinary memory record/result handling for stack-slot-result loads.

Preserve the source idea guardrails: do not special-case `00204.c`,
`myprintf`, `%7s`, `%9s`, `x13`, `sp + 8`, `sp + 15`, one aggregate size, one
branch, or one emitted copy sequence. Do not weaken expectations,
unsupported classifications, runner behavior, timeout policy, CTest
registration, or proof-log behavior.

The first-stage pointer loads are the first concrete break. The later
frame-slot aggregate copy loads/stores still need verification after the temp
bytes are actually materialized; do not assume they are fixed solely because
the initial pointer load path starts emitting.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_instruction_dispatch|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log`.
Build succeeded. Result: 4/5 selected tests passed; the representative
`c_testsuite_aarch64_backend_src_00204_c` still fails with the existing runtime
mismatch. Proof log path: `test_after.log`.
