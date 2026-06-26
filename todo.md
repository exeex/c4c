Status: Active
Source Idea Path: ideas/open/403_prepared_i16_formal_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Publish I16 Formal ABI Facts

# Current Packet

## Just Finished

Step 2 - Publish I16 Formal ABI Facts completed by adding `I16` to the
existing `direct_bir_call_arg_abi_repair()` scalar Integer ABI publication
path in `src/backend/prealloc/legalize.cpp`.

The representative `src/20000403-1.c` no longer fails at the producer-side
ABI-less I16 `unsupported_param_home` admission point. Fresh probe output now
reaches the distinct later RV64 object-route blocker:
`unsupported_scalar_compare_trunc: RV64 object route supports only prepared
named Sge i32 compare results feeding one i16 integer trunc publication`.

## Suggested Next

Address the next RV64 object-route blocker for `src/20000403-1.c` by
generalizing the scalar compare/trunc support needed after I16 formal ABI
publication, without weakening object admission policy.

## Watchouts

- The object-emission admission policy was not changed.
- The new blocker is in scalar compare/trunc object-route support, not formal
  ABI publication.
- Do not special-case `src/20000403-1.c`, `seqgt`, `seqgt2`, or `%p.win`.

## Proof

Ran the supervisor-delegated proof command exactly; backend CTest proof log:
`test_after.log`.

```sh
cmake --build --preset default && mkdir -p build/agent_state && printf '%s\n' 'src/20000403-1.c' > build/agent_state/403_step2_i16_formal.allowlist.txt && { ALLOWLIST=build/agent_state/403_step2_i16_formal.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/403_step2_i16_formal_probe.log 2>&1 || true; } && rg -n 'unsupported_param_home|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ' build/agent_state/403_step2_i16_formal_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build passed; the narrow RV64 probe still reports
`fail	src/20000403-1.c` and `[RV64_C4C_OBJ_COMPILE_FAIL]`, but the failing
shape is now `unsupported_scalar_compare_trunc`, not `unsupported_param_home`.
Backend CTest passed: `100% tests passed, 0 tests failed out of 326`.
