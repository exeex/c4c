Status: Active
Source Idea Path: ideas/open/357_rv64_object_route_data_sections_globals_strings.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative String and Global Cases

# Current Packet

## Just Finished

Completed Plan Step 4: moved `src/20000223-1.c` through RV64 object compile,
link, and qemu execution by teaching prepared RV64 object call lowering to
materialize prepared `SymbolAddress` call arguments with the same PC-relative
data-symbol relocation path used by prepared string/global address
materialization. The repair maps BIR-style prepared symbols such as `@.LC0` to
the emitted object data label before attaching relocations, so string call
arguments target the defined `.rodata` object symbol instead of an undefined
BIR spelling. Added a focused `backend_riscv_object_emission` regression test
for string symbol-address call arguments.

## Suggested Next

Delegate Step 5 milestone validation or lifecycle review. The remaining
allowlisted Step 4 representatives, `src/20000112-1.c` and
`src/20000224-1.c`, now fail in `unsupported_terminator_fragment`, not in data
section, data relocation, or global/string symbol emission.

## Watchouts

- `src/20000227-1.c` is now green in the RV64 object backend allowlist scan.
- `src/20000223-1.c` is now green in the RV64 object backend allowlist scan.
- `src/20000112-1.c` and `src/20000224-1.c` are residual control-flow
  terminator coverage, outside this data-route packet. Do not implement
  general terminator/control-flow lowering under idea 357 Step 4 just to make
  these representative cases pass.
- The new width support is deliberately scoped to global memory helpers; local
  1/2-byte memory remains routed to idea 358.
- Global byte/halfword loads currently encode signed `lb`/`lh`; wider unsigned
  semantics must come from explicit prepared BIR `zext`, which this packet now
  emits. Do not treat raw narrow loads without a suitable consumer as proof of
  unsigned C value semantics.

## Proof

Proof ran:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$' && printf 'src/20000112-1.c\nsrc/20000223-1.c\nsrc/20000224-1.c\nsrc/20000227-1.c\n' > /tmp/rv64-data-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-data-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true
```

The build passed and `backend_riscv_object_emission` passed. The delegated
allowlisted torture scan wrote `test_after.log`; it reports
`src/20000223-1.c` and `src/20000227-1.c` passing, while `src/20000112-1.c`
and `src/20000224-1.c` still fail with `unsupported_terminator_fragment`, for
total=4 passed=2 failed=2. Output is preserved in `test_after.log`.
