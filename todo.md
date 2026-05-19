Status: Active
Source Idea Path: ideas/open/306_aarch64_symbol_offset_address_materialization_width.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Symbol+Offset Address Width Owner

# Current Packet

## Just Finished

Step 1 completed: the owner is the AArch64 scalar control-publication path in
`src/backend/mir/aarch64/codegen/alu.cpp`, specifically
`lower_scalar_select_publication` -> `append_control_value_to_register` ->
`append_move_control_value_to_register` for `MemoryBaseKind::Symbol` operands.

Evidence chain:

- `00050.c.s` has ordinary direct symbol+offset loads using `x9` for `v+8`,
  then the control-publication/select path emits `adrp w9, v+8`, `add w9, w9,
  :lo12:v+8`, and `ldr w9, [w9]`.
- `00176.c.s` has ordinary array stores/loads using `x9`, but the select ladder
  around `array+56` and `array+60` emits `adrp/add/ldr` through `w10` and `w9`
  before `csel w9, w10, w9, eq`.
- `00182.c.s` has the same shape for static-local object offsets
  `__static_local_print_led_3+120` and `+124`: direct stores use `x9`, while
  the select ladder emits symbol+offset materialization through `w10`/`w9`.
- `make_prepared_scalar_load_source` turns prepared global-symbol scalar loads
  into `MemoryOperand{base_kind = Symbol}`. `make_control_publication_operand`
  admits those operands for control publication. `lower_scalar_select_publication`
  derives `result_view` from the scalar select result type (`I32` -> `W`) and
  passes that view plus the `wN` result/scratch name into
  `append_control_value_to_register`. `append_move_control_value_to_register`
  then prints the symbol address materialization using that same `target` for
  `adrp`, `add`, and the memory base, so a 32-bit scalar result register is
  incorrectly reused as an address temporary.
- The normal producer-publication load path in
  `src/backend/mir/aarch64/codegen/dispatch.cpp` already asks for an `X` scratch
  address register before loading into the scalar target, which is the model the
  repair should mirror for this control-publication path.

This covers `00050.c`, `00176.c`, and `00182.c` because all three bad snippets
are I32 scalar select/control-publication loads from same-module global/static
symbol+offset addresses. `00189.c` remains out of scope: the inspected snippet
uses legal `x9`/`x10` address temporaries for `fprintfptr`, `f`, and `stdout`
and does not show this `append_move_control_value_to_register` `wN` address-base
failure.

## Suggested Next

Repair Step 2 in `src/backend/mir/aarch64/codegen/alu.cpp`: change the
`MemoryBaseKind::Symbol` branch of `append_move_control_value_to_register` so
symbol+offset address materialization uses an `X` scratch/base register for
`adrp`, `add :lo12:`, and `[base]`, then loads the scalar value into the
requested `target` view. Preserve `W` targets for the loaded I32 data and for
the following `csel`; do not route through filename/symbol-specific matching.

## Watchouts

- Keep `00189.c` out of scope unless generated-code evidence proves the same
  repair owns externally binding GOT/PIC symbol references such as `stdout`.
- Reject testcase-shaped fixes, exact emitted-instruction matching,
  expectation rewrites, allowlist changes, unsupported classification changes,
  timeout policy changes, runner behavior changes, CTest registration changes,
  or proof-log policy changes.
- Preserve the distinction between 64-bit address temporaries/memory bases and
  32-bit scalar data values loaded from or stored to memory.
- Avoid changing the general address-materialization record path unless the
  repair proves this control-publication inline-assembler path delegates to it;
  direct same-file global loads already use `xN` in the focused generated
  assembly.

## Proof

Inspection-only packet per supervisor proof instruction. No tests were run and
`test_after.log` was not modified.
