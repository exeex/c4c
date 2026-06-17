# Current Packet

Status: Complete
Source Idea Path: ideas/open/303_rv64_runtime_defined_global_scalar_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Lower Zero-Initialized Global i32 Loads

## Just Finished

Completed `plan.md` Step 5: extended the initialized scalar i32
`LoadGlobalInst` path to also lower default-zero scalar i32 globals through the
same RV64 prepared global-memory owner and semantic global label emission.

Implemented load behavior:

- Zero-initialized scalar i32 global loads resolve the prepared memory access by
  block/instruction/result, require default address space, nonvolatile
  direct-global addressing, zero byte offset, size 4, alignment at least 4, and
  base-plus-offset availability.
- The emitted code materializes the semantic global label with `lla` and loads
  the i32 value with `lw` into the prepared result register.
- The same simple-defined scalar i32 global gate now accepts both immediate
  initializers and missing initializers; storage for the missing initializer
  remains `.bss` plus `.zero 4`.
- Registered only `backend_rv64_runtime_global_load_zero_init` after the repo
  RV64 runtime harness executed `global_load_zero_init.c` under qemu with
  expected exit code `0`.
- Global store lowering and `global_store.c` runtime registration were not
  added in this packet.

## Suggested Next

Execute `plan.md` Step 6 by adding scalar i32 global-store lowering through the
RV64 prepared global-memory owner and registering `global_store.c` only after
the real RV64 asm route executes under qemu with expected exit code `7`.

## Watchouts

- Step 6 should use the same semantic global-symbol path and prepared
  memory-access authority for stores; do not introduce testcase-shaped matching
  around `global_store.c`.
- Store lowering still intentionally returns `std::nullopt`; implement it in
  the existing global-memory owner rather than dispatching around the owner.
- The raw load instruction's `align_bytes` can be `0`; keep relying on the
  prepared memory-access record for load size/alignment proof.
- The prepared access symbol id belongs to the prepared name table, while the
  storage label may come from the BIR module global. Preserve that distinction
  when sharing helpers.
- Reject testcase-shaped shortcuts, fallback acceptance, and scope expansion
  into arrays, aggregates, pointer initializers, extern globals, TLS, or
  GOT-heavy work.

## Proof

Ran the supervisor-selected proof successfully; output is captured in
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/global_load_zero_init.c -o /tmp/c4c_global_load_zero_init.s && rg 'g_counter:|\.zero[[:space:]]+4|lla[[:space:]]+[a-z0-9]+,[[:space:]]*g_counter|lw[[:space:]]+[a-z0-9]+,[[:space:]]*0\([a-z0-9]+\)' /tmp/c4c_global_load_zero_init.s
```

Result: build passed, all 25 matching `backend_rv64_runtime` tests passed
including `backend_rv64_runtime_global_load_zero_init`, and the generated RV64
assembly showed `g_counter:`, `.zero 4`, `lla t0, g_counter`, and
`lw t0, 0(t0)`.
