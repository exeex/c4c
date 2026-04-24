Status: Active
Source Idea Path: ideas/open/bir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create The LIR-To-BIR Private Directory Index

# Current Packet

## Just Finished

Completed `plan.md` Step 2, `Create The LIR-To-BIR Private Directory Index`.
Added the private implementation index
`src/backend/bir/lir_to_bir/lowering.hpp`, moved LIR-to-BIR internals behind
that header, and slimmed `src/backend/bir/lir_to_bir.hpp` back to the public
lowering API surface plus the analysis types still exposed by
`BirLoweringResult`.

Updated the LIR-to-BIR implementation files, the facade implementation, and
tests that intentionally probe lowering internals to include the private index
directly. Also completed the reviewer-requested boundary adjustment by removing
the production prealloc dependency on the nested private LIR-to-BIR index:
`src/backend/prealloc/prealloc.hpp` now declares prealloc-owned
`infer_call_arg_abi`, `src/backend/prealloc/legalize.cpp` backs it with the
existing legalize ABI inference logic, and `src/backend/prealloc/regalloc.cpp`
uses that local surface instead of including
`src/backend/bir/lir_to_bir/lowering.hpp` or calling
`lir_to_bir_detail::compute_call_arg_abi`.

## Suggested Next

Execute `plan.md` Step 3, `Validate The Public BIR Header Surface`, by
confirming BIR printer and validation declarations remain centralized in
`src/backend/bir/bir.hpp`, avoiding new thin one-off headers, and updating only
the includes needed to preserve the top-level BIR index model.

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

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
The build completed and the backend subset passed: 97 tests passed, 0 failed,
with 12 disabled tests not run. Proof log: `test_after.log`.
