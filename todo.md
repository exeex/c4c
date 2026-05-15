Status: Active
Source Idea Path: ideas/open/241_inline_asm_clobber_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Clobber Ingress

# Current Packet

## Just Finished

Plan Step 1 - Inventory Clobber Ingress.

Inline-asm clobbers currently enter at source lowering in
`src/frontend/hir/impl/stmt/stmt.cpp`, where `NK_ASM` appends parser clobber
children to `InlineAsmStmt::constraints` as rendered `~{name}` constraint
tokens. That is the only upstream clobber authority found.

Missing upstream authority: `InlineAsmStmt` has outputs, inputs, constraints,
and side effects, but no structured `clobbers` field. `LirInlineAsmOp` also
has no clobber carrier. `src/codegen/lir/hir_to_lir/stmt.cpp` rebuilds
`rendered_constraints` from output and input counts, so clobber-only tokens
from the HIR constraint string are not a durable LIR/BIR fact. BIR then only
sees clobbers if `make_inline_asm_metadata()` encounters a `~{...}` token in
`LirInlineAsmOp::constraints`; it marks that token as
`InlineAsmOperandKind::Clobber` and records
`unsupported_clobber_constraintN`, but it has no structured clobber list to
validate or carry.

## Suggested Next

First implementation packet for Plan Step 2: add a structured clobber carrier
from HIR to LIR to BIR.

Owned code should be limited to the source/LIR/BIR carrier path and its narrow
tests:

- Add `std::vector<std::string> clobbers` to `InlineAsmStmt` and populate it
  from `NK_ASM` parser clobber children instead of appending those facts only
  into `InlineAsmStmt::constraints`.
- Add a matching structured clobber list to `LirInlineAsmOp`, thread it from
  HIR-to-LIR emission, and preserve any rendered `~{...}` constraint spelling
  only as LLVM constraint text rather than as BIR authority.
- Add a BIR inline-asm clobber list carrier, have `make_inline_asm_metadata()`
  populate it from `LirInlineAsmOp` structured clobbers, and keep constraint
  tokens of kind `Clobber` fail-closed unless backed by that structured list.
- Update `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` with a
  supported structured clobber carriage case and a nearby fail-closed case for
  rendered-only `~{...}` clobber tokens.

## Watchouts

- Do not parse clobbers from rendered template text, final assembly, or
  diagnostic strings.
- Treat source parser clobber children as the authority; rendered `~{...}`
  constraint text is compatibility spelling, not ownership.
- Do not make `InlineAsmOperandKind::Clobber` a supported operand by itself.
  The current downstream prepared and AArch64 printer routes intentionally
  reject clobber operands/lists until structured carrier facts exist.
- Preserve the completed inline-asm behavior from closed idea 240.
- Keep memory/address constraints in
  `ideas/open/242_inline_asm_memory_address_constraints.md`.
- Keep alias-aware tied-home allocation policy in
  `ideas/open/243_inline_asm_tied_home_allocation_policy.md`.

## Proof

No build required for this inventory-only lifecycle scratchpad update; proof is
not applicable and no new `test_after.log` was produced.

Suggested proof subset for the first implementation packet:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_printer_test`

`ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_printer)$' | tee test_after.log`
