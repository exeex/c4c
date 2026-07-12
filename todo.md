# Current Packet

Status: Active
Source Idea Path: ideas/open/727_common_prepared_return_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit the stale production inputs

## Just Finished

- Lifecycle repair parked idea 709 after its Step 2.1 consumer change was
  rejected and fully reverted. The attempted helper deletion is not progress;
  focused proof showed real return-chain inputs classify `Stale` and added two
  failures beyond the known baseline.
- Reopened idea 727 because the missing proof-attribution/freshness authority
  belongs to the same common production contract its earlier close claimed to
  complete.

## Suggested Next

- Execute Plan Step 1 by tracing the first stale authority fact across at least
  one-link and multi-link production return-chain inputs.

## Watchouts

- Keep AArch64 implementation unchanged and do not reconstruct target
  semantics, weaken `Stale`, or inject authority only into a named fixture.
- `backend_aarch64_instruction_dispatch` is the recorded baseline failure;
  `backend_aarch64_return_lowering` and the external add/sub-chain smoke failure
  are blocker evidence preserved in `test_after.log`.

## Proof

- Lifecycle-only repair. The rejected packet's exact focused output remains in
  `test_after.log`; fresh implementation proof belongs to the next executor
  packet.
