Status: Active
Source Idea Path: ideas/open/357_rv64_object_route_data_sections_globals_strings.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative String and Global Cases

# Current Packet

## Just Finished

Completed Plan Step 4: moved `src/20000227-1.c` past the residual 1-byte
prepared global-memory width bucket. RV64 object emission now accepts prepared
global-symbol scalar loads/stores of 1, 2, 4, and 8 bytes through a global-only
width helper, leaving local 1/2-byte memory on its existing unsupported route.
The implementation also emits prepared integer `zext`/`sext`/`trunc`/same-width
`bitcast` fragments so the explicit prepared BIR `zext i8 ... to i32` in
`20000227-1.c` supplies the unsigned value semantics instead of guessing from
the global load itself.

## Suggested Next

Delegate the next Step 4/5 packet to classify or repair the remaining
allowlisted representative failures from this proof: `src/20000112-1.c`,
`src/20000223-1.c`, and `src/20000224-1.c`.

## Watchouts

- `src/20000227-1.c` is now green in the RV64 object backend allowlist scan.
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
`src/20000227-1.c` passing and the other three allowlisted representatives still
failing, for total=4 passed=1 failed=3. Output is preserved in `test_after.log`.
