Status: Active
Source Idea Path: ideas/open/618_runtime_mismatch_ownership_investigation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Write Likely First Owner Map

# Current Packet

## Just Finished

Step 3 for `ideas/open/618_runtime_mismatch_ownership_investigation.md`
created `docs/runtime_mismatch_ownership/02_likely_first_owner_map.md` from the
accepted Step 1/Step 2 runtime symptom evidence.

The owner map records:

- The accepted July 9 runtime baseline remains `217` rows: `110`
  abort/assertion, `102` segfault, `0` wrong-output, and `5` timeout rows.
- `src/990106-1.c` is classified as a high-confidence call-lowering owner
  because its log reports an `ld.so` `_dl_fixup` assertion for a non-JMP-slot
  relocation.
- The generic abort family remains unresolved as a first owner and is split
  into ABI, layout, local/global memory, call lowering, and true runtime
  support rerun lanes with representative rows.
- The segfault family remains unresolved as a first owner and is split into
  layout, ABI, local/global memory, call lowering, and true runtime support
  rerun lanes with representative rows.
- Wrong output remains a stable empty family in the accepted baseline.
- The five timeout rows remain unresolved, with control-flow/runtime evidence
  needed before any true runtime support or timeout-policy conclusion.
- Rows/families that should be rerun after prerequisite compile-time or
  codegen ideas are recorded explicitly.

## Suggested Next

Proceed to Step 4 and write
`docs/runtime_mismatch_ownership/03_followup_implementation_queue.md` plus
`docs/runtime_mismatch_ownership/index.md`. The queue should stay owner-first:
call-lowering rerun for `src/990106-1.c`, ABI evidence work, layout evidence
work, local/global memory evidence work, timeout/control-flow investigation,
and true runtime support only for residual rows after those prerequisites.

## Watchouts

The current logs mostly record exit status, not source assertion site, fault
address, qemu trace, or register state. Do not turn generic abort, segfault, or
timeout rows into implementation work without a follow-up evidence pass after
the relevant ABI, layout, local/global memory, call-lowering, or
control-flow/codegen prerequisites.

Keep the source idea's older `75` row estimate separate from the accepted July
9 `217` row runtime baseline.

## Proof

Documentation-only packet. Verified
`docs/runtime_mismatch_ownership/02_likely_first_owner_map.md` exists and maps
runtime families to ABI, layout, local/global memory, call lowering, true
runtime support, or unresolved without collapsing all runtime symptoms into one
bucket. No build or backend proof was run. `test_after.log` was not created or
modified by this packet.
