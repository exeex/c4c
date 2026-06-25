Status: Active
Source Idea Path: ideas/open/351_c4c_objdump_rv64_custom_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Canonical Assembly Printer

# Current Packet

## Just Finished

Step 3 from `plan.md` is implemented: `c4c-objdump` now prints decoded RV64
`.text` as canonical assembly accepted by `c4c-as`. The emitted file contains
`.text`, `.globl main`, `main:`, `.insn.d 10, 11, v6, v0, v2, v4, 3`,
`li a0, 0`, and `ret`. Focused coverage now assembles the objdump output back
through `c4c-as` and proves the reassembled object preserves the canonical
`.text` bytes `0a0320080b0300001305000067800000`.

## Suggested Next

Execute Step 4 from `plan.md`: add the repeated roundtrip proof
`a.o -> a.s -> b.o -> b.s` and compare `a.s == b.s` exactly.

## Watchouts

- Unsupported instruction bytes still fail closed before an output file is
  written.
- The printer uses decoded fields as the semantic source of truth; it does not
  hard-code the full canonical byte string as disassembly.
- The stdout summary still includes extraction hex and decoded instruction
  count for diagnostics, but the `.s` file is now plain assembler input.
- The app-local ELF reader exists because the translated shared
  `linker_common` C++ parser is referenced by backend stubs but not currently
  available as a buildable module.

## Proof

Supervisor-selected proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(c4c_objdump_|c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)' | tee test_after.log
```

Result: passed, 3/3 tests. Proof log: `test_after.log`.
