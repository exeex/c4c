Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Expand Output, Tie, And Clobber Coverage

# Current Packet

## Just Finished

Completed Plan Step 5 by hardening selected and printed AArch64 inline-asm
coverage for output homes, numeric ties, side effects, and explicit clobber
diagnostics without adding allocator-scale behavior.

Implemented:

- Dispatch now reports a distinct fail-closed diagnostic for complete-but-bad
  clobber operands instead of folding them into generic unsupported operands.
- Machine-printer diagnostics now distinguish unsupported clobber operands and
  unstructured clobber lists from ordinary unsupported constraints.
- Prepared-printer coverage now proves source/LIR clobber operands still become
  explicit unsupported-clobber facts.
- Dispatch coverage now uses non-trivial prepared homes (`w3`/`w5`) and checks
  output, tied input, named register input, side-effect marker, selected defs
  and uses, and printed `%wN`/`%xN` substitution from prepared homes.
- Printer coverage now proves tied placeholders print through the selected
  output operand while ordinary operands use their selected homes, and clobber
  operands/lists remain diagnostic-only.

## Suggested Next

Next coherent packet: execute Plan Step 6 broader backend validation and
closure-readiness review for the inline-asm machine-node route.

## Watchouts

- Source/LIR still lacks structured clobber authority for printable clobber
  effects; clobber operands and clobber lists intentionally remain explicit
  diagnostics.
- Allocator/spill work, tied-output coallocation policy, memory constraints,
  named operand references, and broader constraint families remain outside this
  packet and should not be claimed as implemented by Step 5.
- Immediate substitution intentionally emits the raw integer string so templates
  can decide whether to include `#`.
- `clang-format` is not installed in this environment; changed formatting was
  checked manually.

## Proof

Ran the delegated proof command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build plus 139/139 backend tests
passing.
