Status: Active
Source Idea Path: ideas/open/146_call_argument_materialization_call_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Public Call-Argument Materialization Declarations

# Current Packet

## Just Finished

Completed Step 2 declaration ownership move.

- `src/backend/prealloc/calls.hpp` now owns the public
  `PreparedCallArgumentSourceProducerMaterialization`,
  `prepared_call_argument_binary_producer_opcode_is_materializable`, and
  `find_prepared_call_argument_source_producer_materialization` declarations.
- `src/backend/prealloc/prepared_lookups.hpp` no longer declares those symbols
  directly.
- No implementation files were touched in this packet.

## Suggested Next

Step 3 packet: move the corresponding definitions out of
`src/backend/prealloc/prepared_lookups.cpp` and into the call implementation
owner, keeping same-block/source-producer lookup behavior delegated to the
select-chain owner.

## Watchouts

- Keep this route limited to call-argument materialization ownership.
- Do not duplicate same-block materialization logic in call ownership.
- Do not turn the lookup into an AArch64-only shortcut.
- Do not mix ABI move-bundle work into this packet.
- `calls.hpp` cannot include `select_chain_lookups.hpp` directly at top level:
  `publication_plans.hpp` includes `calls.hpp`, while `select_chain_lookups.hpp`
  depends on publication producer types. This packet keeps the public
  materialization type in `calls.hpp` with a producer-type template alias and leaves
  `prepared_lookups.hpp` including `select_chain_lookups.hpp` for the full
  producer definition.
- `src/backend/mir/aarch64/codegen/calls.cpp` and the stack-call contract test
  currently include prepared lookup headers for other APIs too; do not remove
  those includes in the next packet unless the file no longer needs unrelated
  prepared lookup declarations.

## Proof

`cmake --build --preset default` passed. Proof log: `test_after.log`.
