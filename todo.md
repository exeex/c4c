Status: Active
Source Idea Path: ideas/open/357_rv64_object_route_data_sections_globals_strings.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative String and Global Cases

# Current Packet

## Just Finished

Completed Plan Step 4: admitted the next semantic prepared RV64 global memory
instruction shape in object emission and proved the four representative data
allowlist.

The RV64 object route now lowers nonvolatile default-address-space prepared
`LoadGlobalInst`/`StoreGlobalInst` accesses when the prepared memory access
publishes a direct `GlobalSymbol` base, a signed-12-bit byte offset, supported
32-bit or 64-bit scalar size, matching result/stored value identity, and an
in-bounds defined prepared global object. It emits PC-relative `AUIPC`/`ADDI`
object relocations to the global object symbol and then emits the typed
`lw`/`ld` or `sw`/`sd` using the prepared byte offset. Focused object-emission
tests cover admitted global load/store relocations and retain a precise
missing-prepared-access rejection.

Representative allowlist result in `test_after.log`: total=4, passed=0,
failed=4. `src/20000112-1.c` remains at `unsupported_terminator_fragment`;
`src/20000223-1.c` remains at `unsupported_instruction_fragment`;
`src/20000224-1.c` moved past the old global load/store admission blocker to
`unsupported_terminator_fragment`; `src/20000227-1.c` now fails at the narrower
`unsupported_global_data: RV64 object route supports only 32-bit and 64-bit
prepared global memory accesses` bucket.

## Suggested Next

Delegate the next Step 4/5 packet to decide whether the 1/2-byte prepared
global memory width bucket in `src/20000227-1.c` belongs in the object route
now, or whether Step 5 should milestone-scan with that residual classified.
Keep pointer/name initializers, unrelated ABI width work, and expectation
rewrites out of this packet.

## Watchouts

- The new object-route global memory admission consumes prepared memory facts
  only; it does not infer symbol identity, initializer shape, or byte offsets
  from source/BIR names.
- `src/20000224-1.c` proves representative 32-bit prepared global memory now
  gets beyond the previous blanket global instruction bucket, but execution is
  still blocked by terminator lowering.
- `src/20000227-1.c` is the live data-route residual: prepared global memory
  exists, but the object route currently supports only 32-bit and 64-bit scalar
  memory encodings.

## Proof

Proof ran:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$' && printf 'src/20000112-1.c\nsrc/20000223-1.c\nsrc/20000224-1.c\nsrc/20000227-1.c\n' > /tmp/rv64-data-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-data-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true
```

The build and focused `backend_riscv_object_emission` test passed. The
allowlist result is preserved in `test_after.log` and records total=4,
passed=0, failed=4 with the buckets listed above.
