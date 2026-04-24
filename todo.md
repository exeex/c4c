Status: Active
Source Idea Path: ideas/open/bir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Validate The Public BIR Header Surface

# Current Packet

## Just Finished

Completed `plan.md` Step 3, `Validate The Public BIR Header Surface`.
Inspected `src/backend/bir/bir.hpp`, top-level `src/backend/bir/*.hpp`,
`src/backend/bir/bir_printer.cpp`, `src/backend/bir/bir_validate.cpp`, and BIR
include sites under `src/` and `tests/`.

Confirmed BIR printer and validation declarations remain centralized in
`src/backend/bir/bir.hpp` as `std::string print(const Module& module)` and
`bool validate(const Module& module, std::string* error)`. No
`bir_printer.hpp`, `bir_validate.hpp`, or other thin print/validation header
exists.

Removed one stale one-off BIR include from
`src/backend/mir/aarch64/codegen/emit.cpp`: `../../../bir/call_decode.hpp` was
referenced by the include list, but no such tracked or filesystem header exists
and no symbol from it is used there. No BIR behavior, declarations, testcase
expectations, or LIR-to-BIR private boundary files changed.

## Suggested Next

Execute `plan.md` Step 4, `Split Memory Lowering Into A Subdirectory`, by
moving the memory-family lowering implementation files under
`src/backend/bir/lir_to_bir/memory/`, updating CMake and includes, and adding a
memory subdomain index only if shared memory-specific declarations need it.

## Watchouts

- This initiative is structural; do not change lowering semantics or testcase
  expectations.
- Do not edit `ideas/open/bir-agent-index-header-hierarchy.md` for routine
  execution notes.
- Do not create one header per implementation file.
- `BirLoweringResult` still exposes `BirModuleAnalysis`, so
  `BirFunctionPreScan` and `BirModuleAnalysis` remain in the public header for
  this behavior-preserving slice.
- Tests still intentionally include the private index as explicit probes.
- `src/backend/bir/lir_to_bir.cpp` still includes the private index as part of
  the LIR-to-BIR implementation facade; the reviewer-blocked prealloc
  production leak is removed.
- Step 3 left the top-level public header surface flat: tracked
  `src/backend/bir/*.hpp` files are `bir.hpp`, `lir_to_bir.hpp`, and
  `lir_adapter_error.hpp`; the nested private index is
  `src/backend/bir/lir_to_bir/lowering.hpp`.
- For Step 4, keep the memory split structural. Do not create
  `memory/memory.hpp` unless the moved memory files have enough shared
  declarations to justify a subdomain index.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
The build completed and the backend subset passed: 97 tests passed, 0 failed,
with 12 disabled tests not run. Proof log: `test_after.log`.
