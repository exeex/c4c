Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate Dispatch And Shared Prepared Behavior

# Current Packet

## Just Finished

Step 5 - Repair Local-Slot Address Materialization Authority completed by audit;
no code repair was needed. In `emit_value_publication_to_register`, the add/sub
pointer path first delegates to `emit_local_slot_address_publication_to_register`
before falling back to generic recursive binary materialization. The delegated
helper resolves the named base through `prepared_named_value_id` and requires
`prepared_frame_address_offset_for_value`, backed by indexed prepared frame-slot
address materialization facts, before emitting the frame-address `add`.
`local_slot_address_frame_offset` remains a null stub, so the audited
publication route is not using slot-name parsing or local offset reconstruction
as authority.

Lifecycle decision: idea 49 is not closure-ready yet. Step 6 remains as the
active acceptance packet because the canonical `test_before.log` and
`test_after.log` currently record only the Step 5 four-test focused proof, not a
final validation scope covering all repaired dispatch and shared prepared
behavior. The close gate is therefore unmet.

## Suggested Next

Run Step 6 validation as a final no-code packet. Acceptance requires:

- build first with `cmake --build --preset default`;
- focused proof covering dispatch value publication, load-local, load-global,
  select-chain, and local-slot address materialization routes touched or audited
  by Steps 2 through 5;
- broader backend proof using matching canonical `test_before.log` and
  `test_after.log` scope, or existing canonical logs explicitly replaced with
  that scope;
- after broader backend proof, failures must be limited to the known
  `backend_aarch64_instruction_dispatch` and
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`
  failures, with no new focused failures and no new backend regressions;
- `todo.md` must record the exact commands and log locations before lifecycle
  closure is requested again.

## Watchouts

- `emit_local_slot_address_publication_to_register` and the backing helper live
  in `dispatch_publication.cpp`, outside this packet's owned edit set; this
  packet audited them as context and did not modify them.
- Recent supervisor broader backend validation was reported as 165/167 with
  only the known `backend_aarch64_instruction_dispatch` and
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`
  failures, but that result is not represented in the current canonical
  before/after logs.
- This lifecycle packet intentionally did not touch implementation files,
  `plan.md`, idea files, or test logs.

## Proof

Ran fixed proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_cli_dump_prepared_bir_local_arg_call_contract|backend_cli_dump_prepared_bir_vla_goto_stackrestore_cfg|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value)$') > test_after.log 2>&1`

Result: passed. `test_after.log` exists and records the build plus 4/4 passing
focused tests.

Close-gate check: current canonical logs are not sufficient for closure. Running
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
on the current four-test logs reported 4/4 before and 4/4 after, with no new
failures, but default guard status was FAIL because the pass count did not
strictly increase. More importantly, those logs do not cover Step 6 final
validation.
