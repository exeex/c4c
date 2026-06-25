# RV64 c-testsuite asm/objdump roundtrip scan

This is an opt-in RV64 backend scan for a small reviewed subset of
`tests/c/external/c-testsuite`. It is intentionally exposed as a custom CMake
target, not a CTest, so normal `ctest` runs do not pick up the broader
c-testsuite probe by default.

Run it manually from the repository root after configuring `build/`:

```sh
cmake --build build --target rv64_c_testsuite_asm_roundtrip_scan
```

The target builds `c4cll`, `c4c-as`, and `c4c-objdump`, then runs each source
listed in `rv64_c_testsuite_asm_roundtrip_allowlist.txt` through:

1. `c4cll --codegen asm --target riscv64-linux-gnu`
2. `c4c-as`
3. `c4c-objdump`
4. `c4c-as`
5. `c4c-objdump`
6. `c4c-as`

The scan rejects backend fallback text, missing stage outputs, unstable
disassembly text, and unstable reassembled object bytes.

The allowlist is deliberately small. Before adding more c-testsuite cases,
classify nearby unsupported families in
`rv64_c_testsuite_asm_roundtrip_unsupported.md` so the scan grows by reviewed
backend and assembler coverage rather than by accidental testcase fit.

To confirm the scan remains outside default CTest membership:

```sh
ctest --test-dir build -N | rg 'rv64_c_testsuite_asm_roundtrip_scan|c_testsuite_asm_roundtrip_scan'
```

That command should print no registered test for the scan.
