Status: Active
Source Idea Path: ideas/open/561_prepared_local_address_base_plus_offset_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Prepared/RV64 Boundary

# Current Packet

## Just Finished

Step 2 classified the prepared/RV64 boundary for the reproduced
`src/20000519-1.c` local-memory failure from
`build/agent_state/561_step1_20000519_1/`.

First-owner conclusion: RV64-owned. Prepared is not missing the local-memory
access fact for the failing instruction, and the `%t0` stack-home publication is
not incoherent by itself. The first rejecting owner is the RV64 prepared
local-memory consumer, which currently accepts pointer-value base-plus-offset
only when the pointer base already has a register home instead of materializing
the published stack-home pointer value.

Same-run evidence:

- Semantic BIR keeps the failing access as `bar`, `block_1`, instruction 2:
  `%t2 = bir.load_local i32 %t2.addr, addr %t0`.
- Prepared BIR publishes the matching access as
  `access block=block_1 inst_index=2 base=pointer_value result=%t2 pointer=%t0 offset=0 size=4 align=4 base_plus_offset=yes layout_authority=opaque_compatibility range_verdict=unknown_compatible`.
- Prepared BIR publishes `%t0` in `bar` as a normal stack-home pointer value:
  `home %t0 value_id=2 kind=stack_slot slot_id=6 offset=24` and
  `storage %t0 value_id=2 encoding=frame_slot ... slot_id=#6 stack_offset=24`.
- The RV64 object route rejects with
  `unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing`.

Boundary detail: the RV64 helper `prepared_pointer_value_base_offset` verifies
the access is a default, non-volatile pointer-value base-plus-offset access with
matching size/alignment/offset, then calls
`gpr_register_number_for_value_name_local` for the pointer base. That lookup
returns a register only for prepared homes accepted by
`rv64_prepared_gpr_register_number_for_home`, which requires a `Register` or
`PointerBasePlusOffset` home with a register name. For this failing `bar` value,
prepared published `%t0` as a `StackSlot`, so the RV64 consumer declines the
otherwise-visible prepared access instead of loading the pointer base from
slot `#6`/stack offset `24` into a scratch register and using it.

## Suggested Next

Execute the next RV64-local-memory packet by teaching the RV64 prepared
local-memory consumer to materialize a pointer-value base from a prepared stack
home, then use that scratch register for the existing pointer-value
base-plus-offset load/store path.

## Watchouts

- Keep the repair semantic: do not special-case `src/20000519-1.c`; the route
  should handle prepared pointer-value base-plus-offset accesses whose pointer
  base has a coherent stack-slot home.
- Do not reconstruct local-memory facts from RV64 target-specific instruction
  shapes.
- Do not combine this route with direct-call metadata repair.
- Do not weaken unsupported accounting, expected output, tests, or prepared
  admission contracts.
- Do not use named-case shortcuts for retained torture representatives.
- Keep `review/557_step13_vector_local_memory_review.md` untouched unless the
  supervisor explicitly brings it into scope.
- The object-route diagnostic is still not annotated with a function or
  instruction index; the access tie uses the Step 1 same-run BIR traversal order
  plus the RV64 local-memory helper predicate that rejects stack-home pointer
  bases.

## Proof

Step 2 validation:

```sh
git diff --check -- todo.md && scripts/plan_review_state.py show
```

Result: passed. `scripts/plan_review_state.py show` reported the separate
hook-backed state as `current_step_id` `2` and `current_step_title`
`Classify The Prepared/RV64 Boundary` after supervisor alignment.
