# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Resume Expanded Scan Baseline And Candidate Map

## Just Finished

- Completed Step 1 inspection for the resumed parent idea 334 scan surface.
- Confirmed the active baseline after child 337 remains the expanded 33/33
  object-route scan proof recorded in `test_before.log`.
- Current object-route selectors and representative tests:
  - Backend object-byte helper `c4c_add_backend_codegen_object_test(...)`
    registers `backend_route;object` plus target labels. Current green AArch64
    object-byte scalar scan tests are
    `backend_cli_aarch64_return_zero_writes_elf_obj`,
    `backend_cli_aarch64_return_add_writes_elf_obj`,
    `backend_cli_aarch64_return_add_sub_chain_writes_elf_obj`, and
    `backend_cli_aarch64_two_arg_helper_writes_elf_obj`; RV64 has
    `backend_cli_riscv64_return_zero_writes_elf_obj` plus unsupported-path CLI
    diagnostics.
  - RV64 object runtime helper
    `c4c_add_backend_rv64_object_runtime_case(...)` registers
    `backend_runtime;object;rv64;qemu;case` plus scalar/scan labels. Current
    green object runtime scan tests are `backend_obj_runtime_rv64_return_zero`,
    `backend_obj_runtime_rv64_return_add`,
    `backend_obj_runtime_rv64_two_arg_helper`,
    `backend_obj_runtime_rv64_return_add_sub_chain`,
    `backend_obj_runtime_rv64_local_temp`,
    `backend_obj_runtime_rv64_two_arg_local_arg`,
    `backend_obj_runtime_rv64_two_arg_second_local_arg`,
    `backend_obj_runtime_rv64_two_arg_both_local_arg`, and
    `backend_obj_runtime_rv64_local_arg_call`.
  - Existing asm-route runtime counterparts remain under
    `backend_rv64_runtime_*` with `backend_runtime;rv64;qemu;case` and no
    `object` label. AArch64 asm external-return smokes remain selected by
    `backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke`.
  - Label caveat: bare `ctest -L object` currently overselects labels such as
    `aggregate_subobject`; keep using explicit name regexes or anchored label
    selection for proof subsets.
- Dry-ran likely next backend scalar candidates under `/tmp`. RV64 object
  emission still rejects `param_slot.c`, `two_arg_*_rewrite.c`, and
  `branch_if_eq.c` with `RISC-V backend object route unsupported prepared
  module shape`. AArch64 object emission rejects the local/rewrite cases with
  fixed-frame/local-memory unsupported diagnostics and rejects branch control
  flow as unsupported selected machine instruction. These should not be added
  as scan cases yet.
- Checked c-testsuite surface. Existing c-testsuite backend helper still uses
  `--codegen asm`; adding object-route coverage there would be a new opt-in
  helper/registration, not a default change. Temporary object dry-runs showed
  c-testsuite `src/00001.c` and `src/00002.c` emit native object bytes for
  both RV64 and AArch64, and RV64 links/runs them under qemu with exit 0.
  `src/00003.c` passes RV64 object emission but is AArch64 local/frame-memory
  unsupported, so keep it out of the first c-testsuite object smoke packet.

## Suggested Next

- Step 2 should add an opt-in c-testsuite object smoke packet without changing
  c-testsuite defaults.
- Recommended Step 2 owned files: `tests/backend/CMakeLists.txt` and `todo.md`
  only, reusing existing `run_backend_rv64_object_runtime_case.cmake` and
  `run_backend_codegen_object_case.cmake` helpers.
- Add RV64 object-runtime smoke registrations for
  `tests/c/external/c-testsuite/src/00001.c` and `src/00002.c`, with names like
  `backend_obj_runtime_rv64_cts_00001` and
  `backend_obj_runtime_rv64_cts_00002`, expected run code 0, and labels
  including `c_testsuite object backend_runtime rv64 qemu smoke scan`.
- Add AArch64 object-byte registrations for the same two c-testsuite files,
  with names like `backend_cli_aarch64_cts_00001_writes_elf_obj` and
  `backend_cli_aarch64_cts_00002_writes_elf_obj`, expected machine `b700`, and
  labels including `c_testsuite object output_contract dual-route cli smoke
  scan`.
- Suggested Step 2 proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- Keep backend scalar `param_slot.c`, `two_arg_*_rewrite.c`, branch/control
  flow, local arrays/pointers, globals/data, aggregates, broad AArch64
  frame/local-memory expansion, AArch64 runtime, x86 object output, object
  stdout, c-testsuite object defaults, and semantic-BIR object mode out of the
  first c-testsuite object smoke packet.
- Do not introduce c-testsuite object output as a default backend mode in Step
  2. The recommendation is explicit opt-in smoke coverage over known-green
  c-testsuite sources only.
- If Step 2 wants label-only selection, avoid bare `-L object`; it matches
  labels containing `object`, including `aggregate_subobject`.

## Proof

- No build required for this inspection-only packet.
- Ran temporary `./build/c4cll --codegen obj ...` probes with outputs under
  `/tmp` only for backend and c-testsuite candidate mapping.
- Ran `git diff --check`; result: passed.
