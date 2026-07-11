Status: Active
Source Idea Path: ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Generate Ordered Follow-Up Ideas

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by generating the ordered first-wave follow-up
source ideas under `ideas/open/` from
`docs/lir_bir_adapter_boundary/ordered_followup_plan.md` and the Step 3
handoff docs. The generated ideas are dependency ordered:
`685_lir_import_context_extraction.md`,
`686_private_detail_header_contraction.md`,
`687_structured_layout_bridge_isolation.md`,
`688_initializer_lowering_bridge_isolation.md`,
`689_memory_address_provenance_import_cleanup.md`, and
`690_call_abi_import_boundary_cleanup.md`.

## Suggested Next

Execute `plan.md` Step 5 as a closure readiness check. Verify that the
handoff docs and generated follow-up ideas satisfy the umbrella source idea's
acceptance criteria, then prepare a closure note naming the evidence used,
docs written, generated idea order, and intentionally deferred downstream
responsibilities.

## Watchouts

- Step 4 created source ideas only; it did not edit implementation files,
  tests, expectations, unsupported markers, allowlists, root-level proof logs,
  or `docs/lir_bir_adapter_boundary/`.
- The ordered follow-up plan's six first-wave families are represented one to
  one, including the private detail header contraction split documented by the
  Step 3 plan.
- Keep `ValueMap`, `GlobalTypes`, `TypeDeclMap`, `FunctionSymbolSet`, local
  slot/pointer maps, structured layout fallback maps, CFG/phi scratch maps, and
  `memory_types.hpp` side tables import-local unless a later idea proves a
  narrower adapter contract.
- BIR route records and public query surfaces are canonical BIR semantic model
  authority; prepared plans, homes, frame/stack/call/storage products,
  carriers, wrappers, and MIR consumers stay prepared/prealloc or target
  handoff authority.

## Proof

Passed: `git diff --check -- todo.md ideas/open`.
No `test_after.log` is expected because this lifecycle/source-idea packet has
no implementation changes and root-level proof logs are out of scope.
