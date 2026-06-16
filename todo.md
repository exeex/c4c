Status: Active
Source Idea Path: ideas/open/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair the Exposed `stdarg` Direct-Call Blocker

# Current Packet

## Just Finished

Step 3 repaired the exposed `stdarg` direct-call semantic family blocker. The
first rejected call was an AArch64 variadic direct `myprintf` call whose
aggregate HFA operands arrived as ABI carrier arrays such as
`[4 x fp128] alignstack(16) %t43` instead of pointer `byval(...)` operands.

The implementation is in `src/backend/bir/lir_to_bir/calling.cpp`: direct and
indirect typed-call lowering now strips trailing `alignstack(...)` from call
argument type classification, admits matching no-`StructNameId` aggregate
carrier arrays through the existing aggregate-layout path, and expands
AArch64 variadic homogeneous-FP aggregate carriers into scalar FP lane
arguments with lane-count/index ABI metadata. This preserves the prepared
AArch64 HFA overflow publication contract. `tests/backend/bir/CMakeLists.txt`
was refreshed only for shifted structural snippet ids/indices after semantic
admission; the supported-path checks remain equivalent.

## Suggested Next

Step 4 should add focused capability coverage for AArch64 variadic HFA carrier
array call lowering that does not depend on the `00204.c` fixture name.

## Watchouts

- Do not fix this by changing expected dump text, CTest labels, or support
  classification.
- Do not add shortcuts for `00204.c`, `myprintf`, `movi`, or specific HFA
  struct names.
- The direct-call repair is keyed to AArch64 variadic HFA carrier semantics:
  homogeneous FP aggregate layout plus existing aggregate aliases and
  `alignstack(...)` ABI spelling, not a named source file or callee.
- The test snippet update was required because semantic admission changes BIR
  instruction/value ids while preserving the HFA overflow publication shape.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure
```

Result: build passed; both delegated AArch64 tests passed. The narrow AArch64
target tests no longer fail in `stdarg` / `direct-call semantic family`.
Proof log: `test_after.log`.
