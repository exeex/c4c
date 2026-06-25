Status: Active
Source Idea Path: ideas/open/357_rv64_object_route_data_sections_globals_strings.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative String and Global Cases

# Current Packet

## Just Finished

Completed Plan Step 4: repaired the two stale full-suite test contracts left by
the accepted RV64 prepared global-memory capability. The CLI object-route case
for `riscv64_zero_aggregate_global_storage.c` now expects successful RISC-V ELF
object emission instead of an unsupported global diagnostic, and the object
model facade test now checks that RV64 object emission succeeds for a module
with defined global data without falling back to assembly text.

## Suggested Next

Delegate the next Step 4/5 packet to decide whether the 1/2-byte prepared global
memory width bucket in `src/20000227-1.c` belongs in the object route now, or
whether Step 5 should milestone-scan with that residual classified.

## Watchouts

- The CTest name `backend_cli_riscv64_unsupported_global_diagnostic_obj` remains
  stable because the supervisor-selected proof regex names it explicitly, but
  its contract is now a positive object-emission smoke.
- No unsupported expectation was weakened for a still-unsupported path; this
  packet only updated cases now covered by the Step 4 RV64 global-data support.
- `src/20000227-1.c` remains the live data-route residual from the prior packet:
  prepared global memory exists, but the object route currently supports only
  32-bit and 64-bit scalar memory encodings.

## Proof

Proof ran:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_cli_riscv64_unsupported_global_diagnostic_obj|backend_object_model_records|backend_riscv_object_emission)$'
```

The delegated proof passed: 3/3 tests passed, covering the cleaned-up CLI
object-route contract, `backend_object_model_records`, and
`backend_riscv_object_emission`. Output is preserved in `test_after.log`.
