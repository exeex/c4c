Status: Active
Source Idea Path: ideas/open/614_rv64_pointer_local_memory_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Pointer Local-Memory Ownership

# Current Packet

## Just Finished

Executed Step 1, `Refresh Pointer Local-Memory Ownership`, as a
diagnostic-only packet.

Current scan evidence under
`build/rv64_gcc_c_torture_backend/*/case.log` contains `35`
`unsupported_local_memory_access` rows:

- `32` rows stop at the RV64 local-memory consumer diagnostic
  `requires prepared frame-slot or pointer-value base-plus-offset local memory
  addressing`.
- `3` rows stop at the unsupported-width guard
  `supports only 1-, 2-, 4-, and 8-byte prepared local memory accesses`:
  `src/20010605-2.c`, `src/20040208-1.c`, and `src/ieee/inf-1.c`.

Ownership split:

- RV64 local-memory consumer with complete selected frame-slot authority:
  sampled rows `src/20000722-1.c`, `src/20010123-1.c`,
  `src/20011109-2.c`, and `src/920429-1.c` all publish prepared
  `access ... base=frame_slot ... frame_slot=#... offset=... size=...
  align=... base_plus_offset=yes` rows. Several also publish selected
  producer freshness for store-local publication sources. This is the first
  same-family Step 2 candidate.
- RV64 pointer-value memory-use consumer with complete freshness authority:
  mixed rows such as `src/20000722-1.c` and `src/20011109-2.c` also show
  `access ... base=pointer_value ... base_plus_offset=yes`, but this refresh
  did not isolate a standalone pointer-value memory-use family with selected
  freshness authority that is cleaner than the frame-slot family. Keep
  pointer-value base materialization as an adjacent guard/Step 3 candidate,
  not the first packet.
- Missing selected authority or BIR GEP/address producer gaps: the documented
  current map still has BIR local-memory producer rows outside this route,
  including load semantics, store semantics, GEP semantics, mixed
  scalar/local-memory semantics, and alloca local-memory semantics. Example
  guards: `src/20041124-1.c`, `src/20010605-2.c`,
  `src/ieee/pr72824-2.c`, and `src/20020411-1.c` when their first diagnostic
  is semantic producer failure rather than RV64 object consumption.
- Direct pointer arithmetic policy and unrelated owners: current
  `unsupported_pointer_arithmetic` policy rows are `src/20021120-1.c`,
  `src/990524-1.c`, `src/loop-2c.c`, `src/pr27073.c`, and
  `src/pr38212.c`. Branch/select/ABI/runtime owners remain out of scope for
  idea 614 and should be used only as guards when relevant.

## Suggested Next

Proceed to Step 2 with the scalar prepared frame-slot local-memory consumer
packet. The packet should lower RV64 load/store local-memory accesses only
when the prepared access explicitly provides `base=frame_slot`, frame-slot id,
byte offset, size, alignment, `base_plus_offset=yes`, default address space,
and a supported 1-, 2-, 4-, or 8-byte scalar/floating width.

Representative positives: `src/20000722-1.c`, `src/20010123-1.c`,
`src/20011109-2.c`, and `src/920429-1.c`.

Negative guards: unsupported-width rows `src/20010605-2.c`,
`src/20040208-1.c`, `src/ieee/inf-1.c`; BIR producer rows
`src/20041124-1.c`, `src/ieee/pr72824-2.c`, and `src/20020411-1.c`;
direct pointer-arithmetic policy rows `src/20021120-1.c`,
`src/990524-1.c`, `src/loop-2c.c`, `src/pr27073.c`, and
`src/pr38212.c`; unrelated branch/select/ABI/runtime rows.

## Watchouts

- Consume only explicit selected pointer/local-memory authority from the
  prepared/MIR facts.
- Keep BIR GEP/address production, direct pointer arithmetic policy, ABI,
  branch/select, runtime, expectation, unsupported-marker, allowlist, timeout,
  and accounting work out of this route.
- Treat representative rows as evidence, not testcase-shaped implementation
  contracts.
- Do not infer a frame-slot offset from final assembly shape or testcase name.
  Step 2 should require a prepared memory access record and preserve the
  existing unsupported-width diagnostic for non-1/2/4/8-byte accesses.
- Do not fold pointer-value base materialization into the first frame-slot
  packet unless the implementation can prove the same complete selected
  pointer-value freshness boundary without guessing.

## Proof

Delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed; `test_after.log` reports `100% tests passed, 0 tests failed
out of 346`.
