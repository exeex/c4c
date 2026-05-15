Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Resolve Tied Output Coallocation Boundaries

# Current Packet

## Just Finished

Plan Step 5 - Resolve Tied Output Coallocation Boundaries: selected AArch64
inline-asm tied inputs now require the prepared tied input and output homes to
name the same concrete register before dispatch accepts the carrier. Missing
tied input homes, missing tied output indexes, allocator-dependent homes
without concrete register names, and mismatched tied homes fail closed with
explicit diagnostics; selected-printer guards also reject malformed tied-home
records instead of printing through an unproven output.

## Suggested Next

Next coherent packet: Step 6 remaining-scope validation and lifecycle decision
for the active AArch64 inline-asm runbook, including whether broader backend
proof is enough to request close or whether clobber, memory/address, or
allocator-policy leftovers need durable follow-up ownership.

## Watchouts

- Do not touch `plan.md` or `ideas/open/240_aarch64_inline_asm_machine_nodes.md`
  for routine packet progress.
- This slice did not add allocator coallocation policy, scratch allocation, or
  spill behavior; it only trusts homes that prepared records already make
  concrete.
- Tied homes compare the prepared register-name authority directly. If future
  work needs alias-aware `wN`/`xN` coallocation, that should be an allocator or
  prepared-home policy packet rather than a printer shortcut.
- Clobbers and memory/address constraints still need upstream/source-LIR,
  prepared-home, or allocator ownership before real support.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_instruction_dispatch|aarch64_machine_printer|prepared_printer)'; } > test_after.log 2>&1`.
Result: passed; `test_after.log` contains the build and three-test focused
subset output.
