Status: Active
Source Idea Path: ideas/open/351_c4c_objdump_rv64_custom_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repeated Roundtrip Stabilization

# Current Packet

## Just Finished

Step 4 from `plan.md` is implemented: the focused `c4c-objdump` suite now
proves repeated canonical RV64 roundtrip stabilization with
`a.o -> a.s -> b.o -> b.s`, compares `a.s == b.s` exactly, and still verifies
the printed assembly contains `.insn.d 10, 11, v6, v0, v2, v4, 3`,
`li a0, 0`, and `ret`.

## Suggested Next

Supervisor should decide whether idea 351 is ready for lifecycle closure or
needs an independent route review before closing.

## Watchouts

- Unsupported instruction bytes still fail closed before an output file is
  written.
- The repeated roundtrip check is attached to the existing success helper, so
  any future supported success case must also stabilize its printed assembly.
- No `src/apps/c4c-objdump.cpp` change was needed; the current printer output
  was already stable under the repeated roundtrip.

## Proof

Supervisor-selected proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(c4c_objdump_|c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)' | tee test_after.log
```

Result: passed, 3/3 tests. Proof log: `test_after.log`.
