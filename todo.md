Status: Active
Source Idea Path: ideas/open/309_rv64_aggregate_global_storage.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Recheck Representative Bucket Cases

# Current Packet

## Just Finished

Step 5 rechecked the delegated RV64 aggregate-global representative bucket.
`build/rv64_aggregate_global_recheck/summary.md` records outcomes for
`src/00024.c`, `src/00047.c`, `src/00048.c`, `src/00050.c`, `src/00091.c`,
`src/00115.c`, `src/00146.c`, `src/00148.c`, `src/00163.c`, and `src/00176.c`.
`src/00024.c` now emits past the old aggregate global storage/offset-access
failure mode with `.bss`, `.zero 8`, `lla`, `sw`, and `lw` for global `v`.
Nearby visible outcomes: `00047`, `00048`, `00050`, `00091`, `00115`, `00146`,
and `00148` still fail at `riscv prepared module emitter does not support this
prepared global storage layout`; `00163` and `00176` emit assembly, with
`00176` showing `.bss`, `.zero 64`, and repeated `lla`/`sw` stores into global
`array`.

## Suggested Next

Supervisor should choose the next packet from the recorded recheck evidence.
No new repair was attempted in this evidence-only packet.

## Watchouts

- `tests/c/external/c-testsuite/src/00024.c` now emits `.bss` storage plus
  `lla`/`sw`/`lw` body code for offsets 0 and 4 in a spot check.
- Direct pointer-style global address materialization remains on the existing
  scalar-defined-global predicate; this packet did not add pointer global,
  floating global, string, external-call, or nonzero aggregate initializer
  behavior.
- The selected RV64/RISC-V subset still includes the known pre-existing
  `backend_riscv_prepared_edge_publication` failure.
- The nearby bucket candidates remain mixed evidence only; do not infer full
  pointer, floating, string, external-call, or full-bucket support from these
  spot checks.

## Proof

Ran the delegated proof command exactly:
`cmake --build build --target c4c_backend_tests c4cll -j 2 > /tmp/c4c_step5_agg_build.log 2>&1 && rm -rf build/rv64_aggregate_global_recheck && mkdir -p build/rv64_aggregate_global_recheck && { printf '# RV64 Aggregate Global Representative Recheck\n\n'; for case in 00024 00047 00048 00050 00091 00115 00146 00148 00163 00176; do src="tests/c/external/c-testsuite/src/${case}.c"; out="build/rv64_aggregate_global_recheck/${case}.s"; err="build/rv64_aggregate_global_recheck/${case}.err"; printf '## src/%s.c\n\n' "$case"; if build/c4cll --codegen asm --target riscv64-linux-gnu "$src" -o "$out" >"$err" 2>&1; then printf 'status: emit-ok\n'; { rg -n '(^[A-Za-z_][A-Za-z0-9_]*:|\.bss|\.data|\.zero |\.text|^main:|lla |lw |sw |lb |sb |call |does not support|unsupported)' "$out" || true; } | sed 's/^/asm: /'; else status=$?; printf 'status: emit-fail exit=%s\n' "$status"; sed -n '1,40p' "$err" | sed 's/^/diag: /'; fi; printf '\n'; done; } > build/rv64_aggregate_global_recheck/summary.md && ctest --test-dir build -j --output-on-failure -R 'backend_.*(rv64|riscv).*' > test_after.log 2>&1`.

Build step succeeded and the recheck summary was generated at
`build/rv64_aggregate_global_recheck/summary.md`. CTest selected 37 tests; 36
passed, including `backend_codegen_route_riscv64_aggregate_global_field_access`
and `backend_codegen_route_riscv64_zero_aggregate_global_storage`. The command
exited nonzero only because the known pre-existing
`backend_riscv_prepared_edge_publication` test failed. Canonical proof log:
`test_after.log`.
