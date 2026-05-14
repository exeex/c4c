Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Establish AArch64 Header And Module Skeleton

# Current Packet

## Just Finished

Step 2: Establish AArch64 Header And Module Skeleton recreated
`src/backend/mir/aarch64/module/module.hpp` and
`src/backend/mir/aarch64/module/module.cpp` as compact Stage 3 skeleton files
after the legacy implementation was removed. The new public surface exposes
`BuildResult`, `Module`, `FunctionRecord`, the common MIR carrier aliases, and
`build(const prepare::PreparedBirModule&)`; `build` validates the AArch64 ABI
handoff and returns an empty canonical module product until real lowering is
reintroduced.

## Suggested Next

Delegate the next AArch64 module packet to add the first real canonical
function traversal seam that produces `MachineFunction`/`MachineBlock`
contents without restoring the legacy flat record assembler.

## Watchouts

- The new skeleton intentionally does not lower prepared functions into machine
  nodes, so public assembly paths that require printable nodes should still
  fail closed until a later packet adds semantic lowering.
- `Module::functions` is currently a plain compatibility vector for existing
  backend call sites; a later API cleanup can move users to the canonical
  `Module::mir` carrier or `CompatibilityProjection`.
- Keep `module.hpp` as the single non-helper public header unless lifecycle
  repair authorizes a different layout.
- Do not use cached display strings, source spellings, broad public records,
  raw prepared/source views, or register strings as semantic lowering
  authority.
- Do not weaken tests, mark supported paths unsupported, or claim
  expectation-only progress as implementation conversion.

## Proof

Ran:
`cmake --build build --target c4c_backend -j2`

Also ran optional subset:
`ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_prepared_handoff_gate|backend_aarch64_machine_printer'`

Result: passed. Proof output is preserved in `test_after.log`.
