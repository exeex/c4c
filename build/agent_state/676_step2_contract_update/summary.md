# 676 Step 2 Contract Update

Step 2 replaced the stale expected-failure wrapper for
`backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
with the positive object-route contract used by nearby RV64 object rows.

The row now asserts:

- successful `--codegen obj` emission for
  `riscv64_pointer_global_local_publication_live_load_rejection.c`
- RV64 ELF machine bytes (`EXPECTED_MACHINE_LE f300`)
- a stable object byte sequence covering the live pointer reload and short
  load/store/reload path:
  `ld s1,8(sp); lh t1,0(s1); sh t1,18(sp); lh s1,18(sp)`

This is a contract update, not a compiler implementation change. It is allowed
because Step 1 proved the emitted object links with the system RISC-V toolchain
and runs under `qemu-riscv64`, returning the expected zero-initialized global
short value. The test name stays stable, but the CTest behavior now requires
positive object output instead of accepting a fail-closed diagnostic.

Proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection|backend_cli_riscv64_pointer_global_local_publication'
```

Proof log: `test_after.log`
