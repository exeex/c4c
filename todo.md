Status: Active
Source Idea Path: ideas/open/330_aarch64_non_hfa_aggregate_va_arg_materialization.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Localize Missing Ordinary Dispatch Of Prepared Va Arg Memory Operations

# Current Packet

## Just Finished

Step 2 was split into narrower substeps after the initial repair route
blocked. The general prepared memory-record route for pointer-addressed
`LoadLocalInst` with stack-slot result homes compiles, but it is not the path
currently responsible for the `%7s`/`%9s` copies before the `printf` uses:
those copy instructions still do not appear before `.str31`/`.str33` in
generated `myprintf` assembly.

A narrower call-boundary prepared-addressing experiment can emit the missing
byte-copy shape before the calls, but it changes too many call sites in
`00204.c` and turns the representative into broader output corruption. That
route was backed out as drift/overfit risk. No implementation edits are left
from this packet.

## Suggested Next

Execute plan Step 2.1. Determine why `vaarg.join.14`/`vaarg.join.39`
prepared memory operations are not emitted by ordinary block dispatch before
the call, instead of adding a broad call-boundary replay of prepared memory
accesses.

The next packet should trace those prepared memory records from construction
through the retained ordinary block and identify whether they are unattached,
attached to a skipped/replaced block, filtered by instruction kind, missing
address operands, or scheduled after the observing call.

## Watchouts

The ordinary prepared memory-record builder can represent pointer sources, but
fixing only load-result stack-slot handling is insufficient if those BIR memory
instructions are not being visited/emitted in the retained call block. Avoid
replaying all prepared memory accesses before calls; the experiment emitted the
desired shape locally but widened into unrelated corruption.

Preserve the source idea guardrails: do not special-case `00204.c`,
`myprintf`, `%7s`, `%9s`, `x13`, `sp + 8`, `sp + 15`, one aggregate size, one
branch, or one emitted copy sequence. Do not weaken expectations,
unsupported classifications, runner behavior, timeout policy, CTest
registration, or proof-log behavior.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_instruction_dispatch|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log`.
Build succeeded. Result: 4/5 selected tests passed; the representative
`c_testsuite_aarch64_backend_src_00204_c` still fails with the existing runtime
mismatch. Proof log path: `test_after.log`.
