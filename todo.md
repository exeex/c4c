Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Named Operand Substitution

# Current Packet

## Just Finished

Plan Step 2 - Implement Named Operand Substitution: removed the blanket
AArch64 selected-inline-asm rejection for complete carriers with named
references, resolved `%[name]` placeholders against unique selected operand
names, and substituted named register/tied-output/integer-immediate operands
while keeping malformed, unknown, duplicate, missing-name, clobber, unsupported
constraint, and unsupported modifier cases diagnostic-only.

## Suggested Next

Next coherent packet: tighten selected tied input/output validation so selected
tied inputs only print through the output when the prepared tied input and
output homes agree, leaving allocator-side coallocation policy outside this
backend printer/dispatch slice.

## Watchouts

- Do not touch `plan.md` or `ideas/open/240_aarch64_inline_asm_machine_nodes.md`
  for routine packet progress.
- Source/LIR still marks parsed named operands unsupported in semantic carriers;
  this packet only accepts selected records/carriers that already provide
  matching structured operand-name metadata.
- Clobbers and memory/address constraints still need upstream/source-LIR or
  allocator ownership before real support.
- Tied coallocation must not be "fixed" by printing the output register when
  prepared homes disagree; mismatches should be diagnostics unless allocator
  ownership supplies a shared home.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_instruction_dispatch|aarch64_machine_printer|prepared_printer)'; } > test_after.log 2>&1`.
Result: passed; `test_after.log` contains the build and three-test focused
subset output.
