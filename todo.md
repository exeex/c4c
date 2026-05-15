Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Template Substitution

# Current Packet

## Just Finished

Completed the refined Plan Step 4 AArch64 inline-asm template substitution
slice so supported `%wN` and `%xN` modifiers are reachable through BIR,
prepared carriers, dispatch, and final machine-printer output.

Implemented:

- Changed BIR inline-asm modifier classification so supported AArch64 GP
  width modifiers keep modifier visibility without adding
  `unsupported_template_modifiers`; unsupported modifier forms still add the
  fail-closed fact.
- Kept prepared inline-asm carriers complete when the only template modifiers
  are supported `%wN`/`%xN` forms.
- Removed the dispatch-side blanket rejection of complete modifier carriers,
  allowing supported-modifier carriers to become selected assembler machine
  records.
- Added machine-printer substitution for selected inline-asm assembler records,
  using structured operand records as authority for positional placeholders.
- Tied input placeholders resolve through the selected tied output operand, and
  raw signed integer immediates substitute without fabricating a `#` prefix.
- Supported AArch64 GP register-width modifiers `%wN` and `%xN` print from
  selected register operands.
- Printer diagnostics fail closed for unknown placeholders, unsupported
  modifiers, missing selected operands, missing tie facts, unsupported
  constraints, named operand references, and clobber/unsupported operand kinds.
- Tests now cover supported modifiers through LIR-to-BIR metadata, prepared
  carrier completeness, dispatch selection, and machine-printer substitution,
  plus unsupported modifier diagnostics.

## Suggested Next

Next coherent packet: execute Plan Step 5 by hardening output, tie, clobber,
and side-effect coverage around the selected and printed inline-asm path while
keeping allocator-scale and broader constraint support diagnostic-only.

## Watchouts

- Named operand references, clobbers, memory constraints, allocator/spill work,
  and broader constraint families remain outside this packet.
- Immediate substitution intentionally emits the raw integer string so templates
  can decide whether to include `#`.
- `clang-format` is not installed in this environment; changed formatting was
  checked manually.

## Proof

Ran the delegated proof command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build plus 139/139 backend tests
passing.
