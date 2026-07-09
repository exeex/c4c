Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate The Representative Call-Argument Boundary

# Current Packet

## Just Finished

Completed Step 2 by locating the representative RV64 call-argument boundary
after idea 656. The current evidence remains
`build/agent_state/648_post656_call_evidence/summary.md`: `src/20000722-1.c`
has an explicit `arg.source_selection=local_frame_address_materialization` for
`bar:entry` instruction 10, selected value `%lv._clit_`, slot `#0`, stack
offset `0`, source register `x21` / `s2`, and ABI destination `x0` / `a0`.

The focused text route in
`src/backend/mir/riscv/codegen/prepared_call_emit.cpp` consumes the local
frame-address route in `emit_riscv_simple_call(...)`. Once
`as_local_frame_address_materialization_route(...)` succeeds, it emits direct
`addi <destination_register_name>, sp, <selected offset + active adjustment>`;
for the focused green test this is `addi a0, sp, ...`. If the source selection
claims `LocalFrameAddressMaterialization` but cannot be converted into the
prepared local route, the text route returns `std::nullopt` rather than
falling through to a guessed copy.

The representative/focused object route in
`src/backend/mir/riscv/codegen/object_emission.cpp` reaches
`fragment_for_prepared_call(...)`. Its
`LocalFrameAddressMaterialization` branch calls
`append_rv64_prepared_local_frame_address_call_argument_source(...)`, which
verifies `prepared_frame_slot_address_call_argument_offset(...)` and emits
`addi rd, sp, off`. However, when the prepared argument source encoding is
`Register`, the object route passes the source register as `rd`, then appends
`mv <ABI destination>, <source>`. That is why the representative object
disassembly is `mv s2, sp` followed by `mv a0, s2`, and the focused object
comparison is `mv s1, sp` followed by `mv a0, s1`, while the focused text
route emits direct `addi a0, sp, ...`.

Owned implementation surface for the next packet is the GPR destination branch
inside `fragment_for_prepared_call(...)`, plus the existing verifier/helper
`append_rv64_prepared_local_frame_address_call_argument_source(...)`. The
smallest positive proof target should extend or add object-route coverage for
`tests/backend/case/riscv64_call_arg_local_frame_address_materialization.c`
so object disassembly requires direct address materialization into `a0` and
forbids the two-step materialize-into-source-register plus ABI-copy pattern.

Negative behavior to preserve: keep missing, malformed, ambiguous, non-frame
slot, or non-authorized `LocalFrameAddressMaterialization` selections
fail-closed through the prepared offset verifier; preserve ordinary
register-to-register GPR call-argument lowering when there is no explicit
local-frame-address source selection; do not infer this path from source
spelling, stack offsets, final assembly, testcase identity, or diagnostic text.

## Suggested Next

Execute Step 3 by updating the object-route GPR call-argument branch so an
explicit `LocalFrameAddressMaterialization` source selection materializes the
selected frame-slot address into the ABI destination register. Add or extend a
focused object-route test for
`riscv64_call_arg_local_frame_address_materialization` that proves the direct
`addi a0, sp, ...` object behavior and rejects the stale two-step copy pattern.

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
- Do not reopen idea 656 local-memory policy or string-label pointer admission.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostic text.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `bar`, `s1`,
  `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

No build/ctest proof was required for this diagnostic-only packet. Read
`build/agent_state/648_post656_call_evidence/summary.md`, used
`c4c-clang-tool-ccdb` symbol/signature/callee lookup for
`prepared_call_emit.cpp` and `object_emission.cpp`, and inspected only the
directly related call-argument helpers. Did not create or overwrite
`test_after.log`.
