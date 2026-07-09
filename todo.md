Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Split The Narrow Call-Argument Owner

# Current Packet

## Just Finished

Completed Step 3 by updating the RV64 object-route GPR call-argument branch in
`fragment_for_prepared_call(...)`. An explicit
`LocalFrameAddressMaterialization` source selection now calls
`append_rv64_prepared_local_frame_address_call_argument_source(...)` with the
ABI destination register, so the selected frame-slot address is materialized
directly into `a0` instead of into the source callee-saved home followed by a
copy.

The change preserves the existing fail-closed verifier path:
`append_rv64_prepared_local_frame_address_call_argument_source(...)` still
requires `prepared_frame_slot_address_call_argument_offset(...)` to prove the
selected frame-slot address and adjusted offset. Ordinary register-to-register
GPR argument lowering remains in the later generic register-source branch when
there is no explicit local-frame-address source selection.

Added focused object-route coverage
`backend_cli_riscv64_call_arg_local_frame_address_materialization` for
`tests/backend/case/riscv64_call_arg_local_frame_address_materialization.c`.
The object test requires the RV64 instruction bytes for direct
`addi a0, sp, 0`; local `llvm-objdump` inspection of the generated object shows
the call setup as `mv a0, sp` before `read_local_address`, rather than the old
two-step `mv s1, sp; mv a0, s1` object pattern. The existing focused dump and
text-route tests remain green.

## Suggested Next

Execute Step 4 by proving representative integration for `src/20000722-1.c`:
rerun the focused call-argument assertion and capture fresh RV64 object plus
disassembly evidence showing the representative call receives the selected
local frame-slot address directly in `a0`. Record any downstream owner if the
representative row advances but does not fully pass.

## Watchouts

- Do not rely on pre-656 `mv a0, s2` evidence; refresh the representative row.
- The refreshed representative object evidence is `mv s2, sp` then
  `mv a0, s2`; focused object comparison shows the same shape with `s1`. Do
  not carry forward the stale `mv a0, s1` note for the representative row
  unless a fresh probe reintroduces it.
- The representative text asm output exits 0 but does not reach the `bar` call
  setup before the `foo` label; use the object disassembly for current emitted
  representative call evidence.
- Preserve the `prepared_frame_slot_address_call_argument_offset(...)`
  fail-closed checks; do not bypass them with source-register or stack-offset
  assumptions.
- The focused object test checks direct `addi a0, sp, 0` bytes. If Step 4
  representative proof needs a nonzero selected offset, verify the
  representative disassembly rather than assuming the focused immediate.
- Do not reopen idea 656 local-memory policy or string-label pointer admission.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostic text.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `bar`, `s1`,
  `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

Ran the delegated proof:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_(dump|codegen_route|cli)_riscv64_call_arg_local_frame_address_materialization$"; } 2>&1 | tee test_after.log'`

Result: passed. The build completed and all three focused tests passed:
`backend_dump_riscv64_call_arg_local_frame_address_materialization`,
`backend_codegen_route_riscv64_call_arg_local_frame_address_materialization`,
and `backend_cli_riscv64_call_arg_local_frame_address_materialization`.
Proof log: `test_after.log`.
