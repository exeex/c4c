Status: Active
Source Idea Path: ideas/open/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Add Focused Capability Coverage

# Current Packet

## Just Finished

Step 4 added focused capability coverage for AArch64 variadic HFA carrier-array
call lowering independent of the `00204.c` fixture. The new
`backend_lir_to_bir_notes` case synthesizes a neutral AArch64 variadic direct
call whose aggregate operand is a no-`StructNameId` `[2 x float] alignstack(8)`
carrier array loaded from local aggregate storage, then asserts the lowered BIR
call carries two F32 variadic payload arguments with AArch64 HFA lane
count/index ABI metadata and no byval carrier fallback.

## Suggested Next

Supervisor should choose whether to run Step 5 publication validation and/or
request lifecycle closure review now that the focused capability test and the
existing AArch64 target publication tests are green.

## Watchouts

- The focused test intentionally uses a neutral callee/function name and a
  carrier array type rather than `00204.c`, `stdarg`, `myprintf`, `movi`, or
  fixture struct names.
- `tests/backend/bir/CMakeLists.txt` did not need a new wrapper because
  `backend_lir_to_bir_notes` already covers focused semantic BIR capability
  checks.
- This packet did not touch implementation files; it only proves the Step 3
  semantic repair through an independent local aggregate carrier-array shape.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure
```

Result: build passed; `backend_lir_to_bir_notes` plus both delegated AArch64
target tests passed. Proof log: `test_after.log`.
