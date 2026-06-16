Status: Active
Source Idea Path: ideas/open/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Semantic Admission for the Aggregate `va_arg` Load Shape

# Current Packet

## Just Finished

Step 2 repaired the semantic local-memory admission surface for the captured
AArch64 aggregate `va_arg` route. The implementation is in
`src/backend/bir/lir_to_bir/memory/provenance.cpp`: runtime pointer-address
scalar loads/stores now treat an `i8` pointer-address carrier as opaque
byte-addressable storage even at nonzero byte offsets. This admits the HFA
register-save lane shape reached while lowering aggregate `va_arg` scratch
loads, e.g. `%t111 = load float, ptr %t110` where `%t110` is a fixed-offset
`i8` GEP from a dynamic variadic register-save pointer.

The delegated proof moved both AArch64 target tests past the previous
`myprintf` load local-memory semantic family rejection. The current public
failure is later and separate:
`semantic lir_to_bir function 'stdarg' failed in semantic call family
'direct-call semantic family'`.

## Suggested Next

Execute the next packet against the newly exposed `stdarg` direct-call semantic
family failure, starting from the first unsupported call in the AArch64
semantic route rather than revisiting the now-cleared `myprintf` load
local-memory family.

## Watchouts

- Do not fix this by changing expected dump text, CTest labels, or support
  classification.
- Do not add shortcuts for `00204.c`, `myprintf`, `movi`, or specific HFA
  struct names.
- The Step 2 change intentionally admits runtime `i8` pointer-address carriers
  by semantic address shape, not by fixture, function, or struct name.
- The delegated proof is still red because a later direct-call family blocker
  is now visible; do not treat that as a regression to the load local-memory
  repair.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure
```

Result: build passed; both delegated AArch64 tests still fail, but the failure
moved past `myprintf` load local-memory into
`stdarg` / `direct-call semantic family`. Proof log: `test_after.log`.
