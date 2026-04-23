Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Call Boundary Authority Completion
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Completed another Step 3 "Call Boundary Authority Completion" packet for idea
88 by publishing prepared symbol-address and computed-address call-argument
authority in `PreparedCallPlan`, printing those non-register source shapes
truthfully in prepared dumps, and tightening backend/prealloc tests so
downstream consumers no longer need to recover `@symbol` or base-plus-delta
pointer operands from raw BIR when the prepared call plan already knows them.

Current packet result:
- `populate_call_plans` now preserves symbol-address authority for `@symbol`
  call operands even when regalloc assigned the carrier a register home, so the
  prepared call plan publishes source shape rather than only storage.
- Prepared dump summaries now prefer published symbol and computed-address
  source detail over transient register/stack homes, so callsite summaries show
  `symbol_address:@...` and `computed_address:<base>+<delta>` when available.
- Backend/prealloc tests now prove symbol-address and computed-address
  call-argument authority in both the structured call plan and the prepared
  printer dump.

## Suggested Next

Continue Step 3 by checking whether any remaining call-boundary consumers still
recover aggregate, stack-passed, or indirect-callee pointer shapes from raw BIR
instead of the prepared contract, then publish the next missing prepared
authority directly in `PreparedCallPlan`.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Symbol-address arguments may still carry a register home for move planning;
  treat `source_encoding` plus source detail as the authoritative origin and
  the register/stack fields as transport.
- If later aggregate returns or stack-passed arguments use non-frame-slot or
  multi-lane storage, publish that shape in `PreparedCallPlan` instead of
  sending backends back to raw BIR or storage-plan side channels.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
