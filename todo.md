Status: Active
Source Idea Path: ideas/open/314_rv64_aggregate_local_subobject_and_byval_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Aggregate Residual Evidence

# Current Packet

## Just Finished

Step 1 evidence normalization for idea 314.

Captured BIR, prepared BIR, RV64 assembly, clang/riscv64 link, and qemu output
for:

- `src/00019.c`
- `src/00046.c`
- `src/00140.c`

Artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_314_step1/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_314_step1/summary.md`

All three candidates emit and link, but the emitted assembly is truncated while
codegen still reports success:

- `src/00019.c`: qemu exits `139`; assembly stops after storing the self
  pointer. First un-emitted BIR is chained pointer-value aggregate loads
  through `%lv.s` and final `+8` `i32` field load. Bucket: nested aggregate
  pointer/subobject load flow.
- `src/00046.c`: qemu exits `132`; assembly stops at `mv t0, t0`. Prepared
  addressing marks `%lv.v.4` and `%lv.v.8` field accesses as
  `proven_out_of_bounds` for 4-byte accesses through byte-sized anonymous
  union/nested struct subobject slots. Bucket: nested aggregate subobject
  store/load flow.
- `src/00140.c`: qemu exits `132`; assembly stops before either `call f1`.
  Prepared BIR contains byval aggregate copy lanes and call arg0
  `aggregate_address` with stack-slot ABI destination that simple RV64 call
  emission does not lower. Bucket: aggregate byval/copy and call argument flow.

Selected first repair boundary: make RV64 prepared aggregate-local emission
fail closed instead of returning successful partial assembly, then repair the
nested aggregate local subobject store/load path first. `src/00019.c` and
`src/00046.c` both fail before call ABI execution, so they are better Step 2/3
coverage targets than starting with `src/00140.c` byval/call behavior.

## Suggested Next

Proceed to Step 2 by adding focused backend coverage for:

- local struct self-pointer chains that repeatedly load through an aggregate
  pointer and read a nested field
- anonymous union/nested struct local field stores and reloads that preserve
  4-byte values at aggregate offsets

Keep `src/00140.c` as classified byval/copy/call-argument evidence until the
nested subobject path is repaired or separately guarded.

## Watchouts

- Do not special-case filenames, fixed offsets, or struct/union names.
- Do not reopen generic local frame-slot address publication unless aggregate
  evidence proves it is still incomplete.
- Keep broad vararg and floating aggregate ABI repair out of scope unless it is
  only being classified as a residual.
- The current RV64 emitter can emit/link truncated aggregate-local functions
  and only fail later under qemu; focused tests should catch missing emission
  before relying on qemu symptoms.
- `src/00140.c` includes byval aggregate copy and same-module call argument
  flow; do not fold broad byval/variadic/floating aggregate ABI work into the
  first nested-subobject repair packet.

## Proof

Ran:

- `cmake --build --preset default -j`
- For each of `src/00019.c`, `src/00046.c`, and `src/00140.c`:
  `--dump-bir`, `--dump-prepared-bir`, RV64 asm emit, clang/riscv64 link, and
  qemu run into
  `build/rv64_c_testsuite_probe_latest/triage_314_step1/`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Results:

- Build passed.
- Probe completed and classified all three candidates.
- Delegated backend proof wrote `test_after.log` and failed with the existing
  `backend_riscv_prepared_edge_publication` failure as the only failing
  backend test.
