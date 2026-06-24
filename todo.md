# Current Packet

Status: Active
Source Idea Path: ideas/open/338_aarch64_object_emitter_local_frame_and_scalar_frontier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect AArch64 Local-Frame And Multiply Object Rejections

## Just Finished

- Inspected the AArch64 object route for the first local-frame and multiply
  frontier cases. Parent 334's rolled baseline remains the 37/37 expanded
  object-route scan; this packet made no implementation or test registration
  edits.
- Representative local-frame rejects:
  - `tests/c/external/c-testsuite/src/00011.c` emits a leaf fixed frame with
    `sub sp, sp, #16`, stores immediate zeroes to `[sp,#4]` and `[sp]`, loads
    `[sp]` to `w13`, returns through `x0`, then `add sp, sp, #16`. Object
    emission rejects the frame setup before memory handling with
    `AArch64 object emission supports only selected fixed-size link-register
    frame setup/teardown`.
  - `tests/c/external/c-testsuite/src/00003.c` adds a saved `x20` local-frame
    shape: `str x20, [sp,#8]`, local `str/ldr` from `[sp]`, and
    `ldr x20, [sp,#8]`. It hits the same frame predicate first.
  - `tests/backend/case/param_slot.c` combines a local parameter slot in
    `add_one` with saved `x20`; `main` still uses the already-supported
    link-register frame. The local/callee-save function hits the same frame
    predicate.
  - `tests/backend/case/two_arg_first_local_rewrite.c` is representative of
    the `two_arg_*_rewrite.c` family: `main` uses a 32-byte frame with saved
    `x30` and `x20`, local slot stores/loads, then a direct call. It also
    rejects at the frame predicate before object memory emission.
- Representative multiply reject:
  - `tests/c/external/c-testsuite/src/00012.c` emits selected scalar
    instructions `mov w0,#2`, `add w0,w0,#2`, `mov w9,#2`,
    `mul w0,w0,w9`, `sub w0,w0,#8`, `ret`. Object emission reaches the scalar
    dispatch and rejects `Mul` with
    `AArch64 object emission supports only selected add/sub scalar
    instructions`.
- Target-owned seams:
  - `src/backend/mir/aarch64/codegen/object_emission.cpp`:
    `fragment_for_machine_instruction(...)` dispatches selected machine
    records and currently handles return, call-boundary moves, frame, scalar
    add/sub, and direct calls. It has no `MemoryInstructionRecord` object
    fragment yet.
  - `frame_fragment(const FrameInstructionRecord&)` is the first local-frame
    gate. It currently requires fixed-size link-register preservation and
    rejects leaf stack-adjust-only frames, saved callee registers, frame-pointer
    fixed slots, dynamic stacks, and non-LR callee-save shapes.
  - `scalar_add_sub_fragment(const ScalarAluRecord&)` is the multiply gate.
    It supports selected integer `Add`/`Sub` only; selected `Mul` should be a
    separate scalar object slice unless it stays trivially isolated.
  - The asm printer already models the selected memory shapes through
    `print_memory(...)`, including `FrameSlot` base-plus-offset `ldr/str`
    forms. The object route should mirror that selected record contract rather
    than matching source filenames.

## Suggested Next

- Step 2 should implement the smallest semantic local-frame object slice first:
  leaf fixed-size stack adjustment plus selected frame-slot load/store support
  sufficient for `tests/c/external/c-testsuite/src/00011.c`.
- Candidate Step 2 scan restoration: add
  `backend_cli_aarch64_cts_00011_writes_elf_obj` beside the existing AArch64
  c-testsuite object-byte smokes. Keep `00003.c`, `param_slot.c`, and
  `two_arg_*_rewrite.c` as follow-on local/callee-save or call-frame expansion
  candidates unless their saved-register support remains compact after the
  leaf memory slice.
- Step 2 owned files:
  `src/backend/mir/aarch64/codegen/object_emission.cpp`,
  `src/backend/mir/aarch64/codegen/object_emission.hpp` only if needed,
  `tests/backend/mir/backend_aarch64_object_emission_test.cpp`,
  `tests/backend/CMakeLists.txt`, and `todo.md`.
- Recommended Step 2 proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- Keep AArch64 runtime, broad branch/control-flow, globals/data, pointers,
  aggregates, byval, indirect calls, RV64, x86, object stdout, c-testsuite
  defaults, and semantic-BIR object mode out of this child.
- Keep selected scalar multiply for `00012.c` separate from the first
  local-frame memory slice unless implementation proves it is an isolated
  one-instruction encoder addition with focused tests.
- Parent 334 should resume only after this child repairs the AArch64 target
  object frontier or records a precise blocker.

## Proof

- Inspection-only packet; no build required.
- Focused dry-runs used `/tmp/c4c_338_*` outputs for the five requested
  sources and confirmed the frame and multiply diagnostics above.
- Ran `git diff --check`; result: passed.
