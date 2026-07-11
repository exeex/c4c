Status: Active
Source Idea Path: ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Write The Handoff Documents

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by writing the durable `LIR -> BIR` adapter
boundary handoff documents under `docs/lir_bir_adapter_boundary/`. The new
handoff files normalize the Step 1 interface inventory, Step 2 first-owner
classification, and ordered follow-up plan against the same durable evidence
sources, with `build/` scan paths labeled as transient evidence pointers only.

## Suggested Next

Execute `plan.md` Step 4 by generating ordered follow-up ideas under
`ideas/open/` from the Step 3 handoff plan. The next packet should create the
source ideas for the first adapter-boundary cleanup families while preserving
the Step 3 ownership vocabulary and downstream guardrails.

## Watchouts

- Step 4 owns `ideas/open/` edits; this Step 3 packet did not create source
  ideas.
- The ordered follow-up plan names six first-wave families: LIR import context
  extraction, private detail header contraction, structured layout bridge
  isolation, initializer lowering bridge isolation, memory/address provenance
  import cleanup, and call ABI import boundary cleanup.
- Keep `ValueMap`, `GlobalTypes`, `TypeDeclMap`, `FunctionSymbolSet`, local
  slot/pointer maps, structured layout fallback maps, CFG/phi scratch maps, and
  `memory_types.hpp` side tables import-local unless a later idea proves a
  narrower adapter contract.
- BIR route records and public query surfaces are canonical BIR semantic model
  authority; prepared plans, homes, frame/stack/call/storage products,
  carriers, wrappers, and MIR consumers stay prepared/prealloc or target
  handoff authority.

## Proof

Passed: `git diff --check -- todo.md docs/lir_bir_adapter_boundary`.
Additional non-mutating whitespace scan passed for the three new untracked
handoff docs, since the exact delegated `git diff --check` command does not
inspect untracked files until they are staged or tracked.
No `test_after.log` is expected because this docs-only packet delegated a
direct diff-check proof and restricted root-level proof logs.
