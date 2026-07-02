Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct The First Materialization Packet

# Current Packet

## Just Finished

Completed Step 1, `Reconstruct The First Materialization Packet`, as a
docs-only packet. The first implementation packet is the 130-row
`before_instruction/authority_none/consumer_register_to_stack/register_to_stack_slot`
subqueue from the 151-row `coherent_rv64_mir_materialization` lane.

Durable packet note:
`docs/rv64_gcc_torture_post_contract/move_bundle_materialization_first_packet.md`.

Representative rows for the next implementation proof:

- `src/pr78438.c`: single `i32` register to `i16` stack-slot destination.
- `src/20000121-1.c`: single `i8` register to `i64` stack-slot destination.
- `src/20000801-2.c`: pointer register source to `i32` stack-slot destination.
- `src/20000422-1.c`: three-move bundle whose first move is register to stack.
- `src/20000717-3.c`: two-move bundle in the selected family.

Selected packet counts:

- coherent lane total: 151
- first packet: 130 register-to-stack rows
- later coherent subqueues: 15 immediate-to-stack rows, 3 immediate-to-register
  phi rows, 2 stack-to-stack rows, 1 select-publication row
- same textual register-to-stack shape outside the coherent lane: 5
  prepared-authority rows, excluded by the lane filter
- rerouted out of this idea: 0

## Suggested Next

Delegate Step 2 to inspect and edit
`src/backend/mir/riscv/codegen/object_emission.cpp`, starting at
`fragment_for_prepared_move_bundle` and its `consumer_register_to_stack` branch.
The packet should make RV64 consume prepared register-home to stack-slot-home
facts semantically, including width-changing scalar type pairs when prepared
type facts authorize the required store materialization.

## Watchouts

- Keep the 31 prepared authority rows and the `src/960209-1.c` evidence-gap row
  out of this implementation route.
- Reject filename-specific or testcase-shaped materialization shortcuts.
- Do not weaken expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- The existing RV64 branch already handles a narrow register-to-stack shape but
  rejects selected rows where source and destination scalar sizes differ, and
  may reject multi-move bundles. Treat those as semantic materialization gaps,
  not as permission to guess missing homes or offsets.
- If a row lacks explicit prepared source register, destination stack slot, or
  stack offset authority, reroute it to the prepared-authority idea instead of
  inventing RV64 facts.

## Proof

- Docs-only proof:
  `git diff --check -- todo.md docs/rv64_gcc_torture_post_contract/move_bundle_materialization_first_packet.md`.
- Row-count proof in `test_after.log` confirms the chosen first packet is the
  combined lane-and-shape subset: 130 coherent rows, zero selected
  prepared-authority rows, and zero selected evidence-gap rows.
