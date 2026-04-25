Status: Active
Source Idea Path: ideas/open/09_bir-call-signature-and-abi-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit helper boundaries and wire the new file

# Current Packet

## Just Finished

Step 1 completed the helper-boundary audit and initial wiring. The ABI helper
candidate boundary in `calling.cpp` is the top-of-file cluster made up of
`use_float_return_registers`, `use_float_arg_registers`,
`lower_function_return_abi`, `lower_call_arg_abi`, and the two
`lir_to_bir_detail::compute_*_abi` wrappers. Its current dependencies are
`lowering.hpp`, `c4c::TargetProfile`/`TargetArch`, `bir::TypeKind`,
`bir::AbiValueClass`, `bir::CallResultAbiInfo`, `bir::CallArgAbiInfo`, and
`lir_to_bir_detail::type_size_bytes`. Created
`src/backend/bir/lir_to_bir/call_abi.cpp` with only the normal include and
namespace skeleton, and wired it into `backend_lir_to_bir_notes_test`.

## Suggested Next

Execute the next Step 1/Step 2 packet by moving the audited ABI helper cluster
from `calling.cpp` into `call_abi.cpp` without changing declarations or call
lowering behavior.

## Watchouts

- This is a behavior-preserving extraction; do not rewrite expectations.
- Do not add new headers.
- Keep `lower_call_inst` and `lower_runtime_intrinsic_inst` in `calling.cpp`.
- `calling.cpp` also has unrelated helpers such as link-name resolution,
  function reporting names, `parse_byval_pointee_type`, parameter/signature
  parsing, call lowering, runtime intrinsic lowering, and stack/spill handling;
  those are outside the audited ABI-helper extraction boundary.
- `src/codegen/CMakeLists.txt` uses recursive source discovery; the delegated
  proof log showed the build reconfigured after detecting the new
  `call_abi.cpp` file.

## Proof

Passed:
`cmake --build build --target c4c_codegen && ctest --test-dir build -R "backend_lir_to_bir_notes" --output-on-failure`.
Proof log: `test_after.log`.
