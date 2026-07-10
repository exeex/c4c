# 676 Step 1 Runtime Contract Summary

## Status

Repo-native RV64 runtime/semantic proof path established for
`build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o`.

The object links with repo-native `clang --target=riscv64-linux-gnu
--gcc-toolchain=/usr` and runs under `qemu-riscv64 -L /usr/riscv64-linux-gnu`.
The live-load case returns `0`, matching the zero-initialized global short
value from:

```c
short rv64_pointer_global_local_live_sink;
...
return *loaded;
```

## Commands

Delegated proof command was run into `test_after.log`:

```sh
cmake --build --preset default && {
  ctest --test-dir build -j --output-on-failure -R 'backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection'
  command -v clang
  command -v qemu-riscv64
  readelf -h build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o
  readelf -r build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o
  readelf -s build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o
  objdump -dr build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o
  clang --target=riscv64-linux-gnu --gcc-toolchain=/usr \
    build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o \
    -o build/agent_state/676_step1_runtime_contract/live_load_rejection.bin
  qemu-riscv64 -L /usr/riscv64-linux-gnu \
    build/agent_state/676_step1_runtime_contract/live_load_rejection.bin
  printf 'qemu live-load rejection exit=%s\n' "$?"
  build/c4cll --codegen obj --target riscv64-linux-gnu \
    tests/backend/case/riscv64_pointer_global_local_publication.c \
    -o build/agent_state/676_step1_runtime_contract/publication_known_good.o
  clang --target=riscv64-linux-gnu --gcc-toolchain=/usr \
    build/agent_state/676_step1_runtime_contract/publication_known_good.o \
    -o build/agent_state/676_step1_runtime_contract/publication_known_good.bin
  qemu-riscv64 -L /usr/riscv64-linux-gnu \
    build/agent_state/676_step1_runtime_contract/publication_known_good.bin
  printf 'qemu known-good publication exit=%s\n' "$?"
  objdump -dr build/agent_state/676_step1_runtime_contract/publication_known_good.o
} > test_after.log 2>&1
```

Cross-tool detail appended afterward:

```sh
riscv64-linux-gnu-objdump -dr build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o
riscv64-linux-gnu-objdump -dr build/agent_state/676_step1_runtime_contract/publication_known_good.o
riscv64-linux-gnu-readelf --wide -r build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o
riscv64-linux-gnu-readelf --wide -s build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o
riscv64-linux-gnu-readelf --wide -r build/agent_state/676_step1_runtime_contract/publication_known_good.o
riscv64-linux-gnu-readelf --wide -s build/agent_state/676_step1_runtime_contract/publication_known_good.o
```

## Results

- Build: `cmake --build --preset default` succeeded with no work.
- Focused CTest:
  `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
  failed because the expected-failure wrapper now sees object emission succeed:
  `[BACKEND_OBJ_EXPECTED_FAIL] ... unexpectedly succeeded`.
- Runtime link/run path:
  `build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o`
  linked to `build/agent_state/676_step1_runtime_contract/live_load_rejection.bin`.
- Runtime result:
  `qemu live-load rejection exit=0`.
- Known-good comparison:
  `tests/backend/case/riscv64_pointer_global_local_publication.c` compiled to
  `build/agent_state/676_step1_runtime_contract/publication_known_good.o`,
  linked to `publication_known_good.bin`, and returned
  `qemu known-good publication exit=7`.

## Object Observations

Live-load object:

- Path:
  `build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o`
- ELF header: `ELF64`, little endian, machine `RISC-V`, type `REL`, flags
  `0x5, RVC, double-float ABI`.
- Symbol table includes `main` as an 80-byte global function and
  `rv64_pointer_global_local_live_sink` as a 2-byte global object.
- Relocations are PC-relative global materializations for
  `rv64_pointer_global_local_live_sink`:
  `R_RISCV_PCREL_HI20`/`R_RISCV_PCREL_LO12_I` pairs at text offsets
  `0x8/0xc` and `0x18/0x1c`.
- Cross disassembly shows direct global address materialization, local spill and
  reload, then a live `lh` through the reloaded pointer:

```asm
8:  auipc t1,0x0        # R_RISCV_PCREL_HI20 rv64_pointer_global_local_live_sink
c:  mv    t1,t1         # R_RISCV_PCREL_LO12_I .Lpcrel_hi_global_pointer_local_publication_1_1_0
10: sd    t1,0(sp)
14: ld    s1,0(sp)
18: auipc t1,0x0        # R_RISCV_PCREL_HI20 rv64_pointer_global_local_live_sink
1c: mv    t1,t1         # R_RISCV_PCREL_LO12_I .Lpcrel_hi_global_pointer_local_publication_1_1_2
20: sd    t1,8(sp)
24: ld    s1,8(sp)
28: lh    t1,0(s1)
```

Known-good publication object:

- Path: `build/agent_state/676_step1_runtime_contract/publication_known_good.o`
- Relocations include global pointer publication, store, and load labels for
  `rv64_pointer_global_local_sink`.
- Cross disassembly shows `li t1,7`, a global `sh`, then a global `lh`; qemu
  returns `7`, confirming the same link/run harness observes nonzero global
  short values.

## Harness Notes

No missing RV64 runtime harness blocker remains for this object. The exact
repo-native proof path is:

```sh
clang --target=riscv64-linux-gnu --gcc-toolchain=/usr \
  build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o \
  -o build/agent_state/676_step1_runtime_contract/live_load_rejection.bin
qemu-riscv64 -L /usr/riscv64-linux-gnu \
  build/agent_state/676_step1_runtime_contract/live_load_rejection.bin
echo $?
```

The focused CTest remains red only because its current contract is an
expected-failure wrapper that rejects successful object emission. The host
`objdump` command cannot disassemble this RISC-V object here
(`architecture UNKNOWN`), but `riscv64-linux-gnu-objdump -dr` works and is the
usable repo-native disassembly path.
