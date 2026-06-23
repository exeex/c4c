Status: Active
Source Idea Path: ideas/open/320_rv64_nested_select_chain_store_source_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Nested Store-Source Evidence

# Current Packet

## Just Finished

Completed Step 1 evidence normalization for `src/00144.c`.

Fresh probe artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_320_step1/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_320_step1/summary.md`
- `build/rv64_c_testsuite_probe_latest/triage_320_step1/bir/src_00144.c.bir.txt`
- `build/rv64_c_testsuite_probe_latest/triage_320_step1/prepared/src_00144.c.prepared.txt`
- `build/rv64_c_testsuite_probe_latest/triage_320_step1/asm/src_00144.c.s`
- `build/rv64_c_testsuite_probe_latest/triage_320_step1/bin/src_00144.c.bin`

Fresh status:

- `--dump-bir`: 0
- `--dump-prepared-bir`: 0
- RV64 asm emit: 0
- clang link: 0
- qemu: 0

Classification: `supported-linked-pass-with-prepared-residual`.

The emitted-code failure from earlier evidence is no longer present:
`src/00144.c` reaches `ret`, links, and exits qemu with status 0. However, the
prepared nested store-source records still expose the idea 320 residual:

- `tern.end.23` store source: `status=missing_destination_access`
- `tern.end.33` store source: `status=missing_destination_access`
- `tern.end.54` store source: `status=missing_destination_access`
- `tern.end.63` store source: `status=missing_destination_access`

Prepared `select_chain` records still exist for the corresponding nested join
values (`%t25`, `%t35`, `%t56`, `%t65`), and nearby prepared memory-access
facts exist for the stores/loads around those joins. The first remaining issue
for this route is therefore missing prepared destination-access publication for
nested select-chain store-source records, not a current RV64 link/runtime
failure.

Boundary checks:

- The existing simple pointer-typed select publication neighbor remains green:
  `backend_(dump|codegen_route|rv64_runtime)_riscv64_pointer_typed_select_publication`
  passed.
- This is distinct from idea 321's i16 local-array/select-store residual:
  `src/00144.c` is pointer/integer select-chain publication, not the halfword
  local-array path from `src/00143.c`.

## Suggested Next

Add focused Step 2 expected-repair coverage for nested select-chain
store-source prepared publication. The test should assert the missing
destination-access fact without relying on `src/00144.c`, `tern.end.*`, or
fixed emitted text.

## Watchouts

- Do not special-case `src/00144.c`, `tern.end.*`, fixed source ternary shapes,
  or observed RV64 instruction text.
- Do not treat already-repaired simple pointer-typed select publication as
  completion for this nested store-source route.
- Do not claim implementation progress from this evidence-only packet:
  `src/00144.c` now passes qemu, but the prepared publication residual remains.
- Do not fold idea 321's i16 local-array/select-store residual, stack-homed
  fused compare control flow, aggregate/byval, function-pointer, external-call,
  or broad switch/control-flow work into this route.

## Proof

Probe command sequence ran after `cmake --build --preset default -j`:
`./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00144.c`,
`./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00144.c`,
`./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00144.c -o build/rv64_c_testsuite_probe_latest/triage_320_step1/asm/src_00144.c.s`,
`/usr/bin/clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -x assembler build/rv64_c_testsuite_probe_latest/triage_320_step1/asm/src_00144.c.s -o build/rv64_c_testsuite_probe_latest/triage_320_step1/bin/src_00144.c.bin -lm`,
and `timeout 5s /usr/bin/qemu-riscv64 -L /usr/riscv64-linux-gnu build/rv64_c_testsuite_probe_latest/triage_320_step1/bin/src_00144.c.bin`.

Focused neighbor proof passed:
`ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_pointer_typed_select_publication'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
simple pointer-typed select neighbor tests passed in `test_after.log`, and the
Step 1 `src/00144.c` probe artifacts show emit/link/qemu success with the
prepared nested store-source residual still visible.
