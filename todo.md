Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select AArch64 Inline-Asm Machine Records

# Current Packet

## Just Finished

Completed Plan Step 3 AArch64 inline-asm machine-record selection for complete
prepared carriers without adding template substitution or printer behavior.

Implemented:

- Selected AArch64 assembler machine records for retained inline-asm calls only
  when the matching prepared inline-asm carrier is complete and target-valid.
- The selected record carries raw template text, constraints, side-effect
  state, operands, selected homes/registers/immediates, output/result facts,
  numeric tie facts, placeholder indexes, and retained operand names when
  available from BIR metadata.
- Dispatch fails closed for missing carriers, incomplete carriers, missing
  result/register homes, unsupported clobber/operand kinds, template modifiers,
  and non-AArch64 target profiles.
- Dispatch tests cover the complete selected carrier shape and representative
  malformed/incomplete carriers staying diagnostic-only.

## Suggested Next

Next coherent packet: implement Step 4 template substitution/printing from the
selected structured inline-asm record, still using the raw template and
structured operands as authority instead of formatting during dispatch.

## Watchouts

- The selected record intentionally keeps `%0`/placeholder text unsubstituted;
  no machine-printer behavior changed in this packet.
- Clobbers and template modifiers remain fail-closed. Named operand references
  remain unsupported, though retained operand names are copied when present.
- Inline-asm register operands consume existing prepared GPR homes only; no
  scratch assignment, allocator, spill, or broader constraint family work was
  added.

## Proof

Ran the delegated proof command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build plus 139/139 backend tests
passing.
