Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Establish AArch64 Header And Module Skeleton

# Current Packet

## Just Finished

Step 1: Establish Common Hierarchical MIR Carrier completed the header-level
common MIR skeleton in `src/backend/mir/mir.hpp`. The new common carrier models
module -> function -> block -> vector of instructions, gives machine functions
and blocks first-class BIR identity fields initialized to invalid IDs, and adds
lightweight optional instruction provenance metadata. Legacy `MachineNode`,
`Block<Inst>`, `Function<Inst>`, `empty`, `flatten_instructions`, and
`append_instruction` compatibility remain available for current AArch64 users.

## Suggested Next

Delegate plan Step 2 to begin migrating AArch64 module skeleton code onto the
common carrier shape now that the shared header surface exists.

## Watchouts

- Preserve staged migration; do not delete or disconnect legacy code until the
  replacement owner for that seam is live and proved.
- The pre-existing dirty edits in `src/backend/mir/aarch64/module.hpp` and
  `src/backend/mir/aarch64/module.cpp` were left untouched by this packet.
- Current AArch64 code still uses the legacy `Function<MachineNode<...>>`
  compatibility wrapper; a later migration should decide when to switch those
  aliases to `MachineFunction<TargetInstruction>`.
- Keep `module.hpp` as the single non-helper public header unless lifecycle
  repair authorizes a different layout.
- Do not use cached display strings, source spellings, broad public records,
  raw prepared/source views, or register strings as semantic lowering
  authority.
- Do not weaken tests, mark supported paths unsupported, or claim
  expectation-only progress as implementation conversion.

## Proof

Ran:
`cmake --build build --target c4c_backend -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_prepared_module_identity|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer'`

Result: passed. Proof output is preserved in `test_after.log`.
