Status: Active
Source Idea Path: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar Cast Source Publication Boundary

# Current Packet

## Just Finished

Step 1 localization completed for the `00143` scalar cast source-publication
failure. Prepared BIR has `%t76 = bir.select ... i16`, `%t81 = bir.sext i16
%t76 to i32`, `%t76 value_id=308 kind=stack_slot`, `%t81 value_id=388
kind=register reg=x13`, and a before-instruction consumer move
`from_value_id=308 to_value_id=388 destination_storage=register
reason=consumer_stack_to_register`.

The concrete first bad boundary is
`src/backend/mir/aarch64/codegen/cast_ops.cpp:394`
`make_prepared_consumer_register_source`: the helper is the intended
publication point for a prepared consumer register source, but it returns
early at the `source_home.kind != PreparedValueHomeKind::Register` guard before
looking up the before-instruction move bundle. For this case the source home is
correctly stack-homed, so the available prepared move is never converted into a
structured `RegisterOperand` source for the scalar cast.

This is a prior source-home-kind guard bug at the scalar cast source-publication
boundary. The selected-node construction then exposes the loss: the generic
`lower_scalar_instruction` cast fallback can still publish a selected scalar
cast with the original memory source, and the AArch64 printer correctly rejects
that node because `sign_extend` has no structured register source. This is not
a printer handoff bug, and the prepared move fact itself is present.

## Suggested Next

Step 2 should repair `make_prepared_consumer_register_source` so stack-homed
sources are eligible when a matching before-instruction
`consumer_stack_to_register` move targets the cast result register. Keep the
existing register-source fast path fail-closed, preserve operation gating for
simple integer sign/zero extends, and update the selected scalar cast record's
`source` plus `inputs[0]` before `make_scalar_instruction`.

Coverage surface: focused backend dispatch coverage should assert that a
stack-homed source cast with a prepared consumer stack-to-register move
produces a selected scalar cast carrying a structured register source operand
and prints without the old diagnostic. Reuse the existing
`backend_aarch64_instruction_dispatch` stack-source cast fixture shape if
possible, and add the missing i16-to-i32 sign-extend surface that matches this
failure family without naming `00143` values.

## Watchouts

- Do not special-case `00143`, value ids, registers, source lines, instruction
  numbers, block names, or diagnostic strings.
- `lower_scalar_cast_publication_to_prepared_register` is not the owner for
  this bug: the prepared move already names the exact consumer register for the
  cast, so Step 2 should publish that move as the scalar cast source instead of
  routing through recursive select-chain materialization.
- The generic `lower_scalar_instruction` cast fallback can still create the
  printer-facing invalid shape if the scalar-cast-specific publication helper
  declines the instruction. After the repair, confirm this fallback is no
  longer the first owner for prepared consumer stack-to-register scalar casts.
- Do not frame this as an idea 339 local storage/writeback sizing residual
  unless new generated-code evidence moves the first bad fact there.
- Do not mutate files under `ideas/closed/`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.

## Proof

Non-mutating localization only; no implementation or test files were edited and
`test_after.log` was intentionally not touched by this packet.

- `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c'`
  captured to `/tmp/c4c_00143_ctest.log`; reproduces the old printer failure:
  `scalar cast node requires a structured register source operand`.
- `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu
  --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
  captured to `/tmp/c4c_00143_prepared_main.txt`; confirms value homes and the
  `from_value_id=308 to_value_id=388` consumer stack-to-register move.
- `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu
  --mir-focus-function main --mir-focus-block block_16
  tests/c/external/c-testsuite/src/00143.c` captured to
  `/tmp/c4c_00143_prepared_block16.txt`; confirms the focused block cast shape.
- `./build/c4cll --codegen asm --target aarch64-linux-gnu
  tests/c/external/c-testsuite/src/00143.c -o /tmp/c4c_00143.s` captured to
  `/tmp/c4c_00143_asm.log`; fails before writing assembly with the same
  structured-register-source diagnostic.
- AST/source localization used `c4c-clang-tool-ccdb` on
  `src/backend/mir/aarch64/codegen/cast_ops.cpp`; `lower_scalar_cast_instruction`
  is the only direct caller of `make_prepared_consumer_register_source`.
