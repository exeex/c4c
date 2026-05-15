Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Inline-Asm Carriers

# Current Packet

## Just Finished

Completed Plan Step 2 first structured BIR/prepared inline-asm carrier packet
without selecting AArch64 machine records.

Implemented:

- BIR inline-asm operand metadata for supported register inputs (`r`), one
  register output (`=r`), numeric tied inputs, integer immediates (`i`/`I`),
  clobber placeholders, unsupported constraints, named-reference flags, and
  template-modifier flags.
- LIR-to-BIR constraint-token parsing for structured inline-asm metadata while
  preserving raw template/constraint text and fail-closed unsupported facts.
- Prepared inline-asm carrier structs and population after existing value-home
  authority is available.
- Prepared validation for register input homes, result register homes, tied
  input/result homes, literal or rematerializable integer immediates, operand
  count mismatches, malformed ties, unsupported constraints, named references,
  template modifiers, clobbers, and missing homes.
- Prepared-printer section exposing complete inline-asm carriers and
  missing-fact diagnostics for incomplete carriers.
- Nearby BIR and prepared-printer tests covering supported positional register
  input, one output plus numeric tie, integer immediate input, unsupported
  constraint/name/modifier/clobber diagnostics, and missing-home diagnostics.

## Suggested Next

Next coherent packet: consume complete prepared inline-asm carriers in the
AArch64 selection layer by adding selected machine records only for the same
minimal supported carrier shape, still leaving template substitution,
modifiers, named operands, clobber modeling, allocator/spill work, and broader
constraint families out of scope.

## Watchouts

- The carrier is structural authority only; this packet intentionally did not
  select MIR/AArch64 machine records or substitute templates.
- Named operands and clobbers remain diagnostic-only because source/LIR does
  not provide structured name or clobber vectors.
- Template modifiers are detected and preserved as fail-closed facts; no target
  modifier interpretation is implemented yet.
- Register constraints require existing prepared register homes. No allocator,
  spill planner, scratch convention, or physical-register fabrication was added.

## Proof

Ran the delegated proof command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build plus 139/139 backend tests
passing.
