# Current Packet

Status: Complete
Source Idea Path: ideas/open/303_rv64_runtime_defined_global_scalar_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Lower Global i32 Stores

## Just Finished

Completed `plan.md` Step 6: added scalar i32 `StoreGlobalInst` lowering through
the RV64 prepared global-memory owner and registered `global_store.c` after the
real qemu-backed RV64 runtime case passed with expected exit code `7`.

Implemented store behavior:

- Store lowering resolves the prepared memory access by block/instruction and
  stored value, requires default address space, nonvolatile direct-global
  addressing, zero byte offset, size 4, alignment at least 4, and
  base-plus-offset availability.
- The emitted code materializes the stored i32 value through the existing scalar
  move/materialization helper, materializes the semantic global label with
  `lla`, and stores with `sw` to offset zero.
- Immediate and named i32 store values both route through the same prepared
  owner path; no testcase-shaped matching was introduced.
- Registered only `backend_rv64_runtime_global_store` in this packet.

## Suggested Next

Supervisor should run the acceptance handoff for the completed Step 1-6 runtime
defined-global scalar memory slice and decide whether lifecycle review/closure
or broader regression validation is required.

## Watchouts

- Raw global store alignment can be absent even when the prepared memory access
  proves `align=4`; keep the prepared access record as the authority for the
  lowered memory operation.
- The current owner is intentionally limited to simple defined scalar i32
  globals in the default address space. Arrays, aggregates, pointer
  initializers, extern globals, TLS, nonzero offsets, volatile accesses, and
  GOT-heavy work remain outside this slice.
- The runtime bucket now contains three global scalar cases:
  `global_load.c`, `global_load_zero_init.c`, and `global_store.c`.

## Proof

Ran the supervisor-selected proof successfully; output is captured in
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/global_store.c -o /tmp/c4c_global_store.s && rg 'g_counter:|\.word[[:space:]]+11|lla[[:space:]]+[a-z0-9]+,[[:space:]]*g_counter|sw[[:space:]]+[a-z0-9]+,[[:space:]]*0\([a-z0-9]+\)|lw[[:space:]]+[a-z0-9]+,[[:space:]]*0\([a-z0-9]+\)' /tmp/c4c_global_store.s
```

Result: build passed, all 26 matching `backend_rv64_runtime` tests passed
including `backend_rv64_runtime_global_store`, and the generated RV64 assembly
showed `g_counter:`, `.word 11`, `li t1, 7`, `lla t0, g_counter`,
`sw t1, 0(t0)`, and `lw t0, 0(t0)`.
