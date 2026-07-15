# Current Packet

Status: Active
Source Idea Path: ideas/open/794_lir_next_local_vla_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Write the exact 734 handoff and obtain acceptance disposition

## Just Finished

- 794 Step 2 confirmed closed 798's selected native `LirStackRestoreOp`
  authority without a new repair: selected admission, `saved_ptr` binding,
  current-function object/owner/pointer/pointee/live authority, and the
  `RestoreSavedVlaStackCheckpoint` transition are emitted and verified.
  Nearby producer/verifier proof covers the valid selected restore and
  malformed, foreign, type-incoherent, non-live, unbound, and
  transition-invalid rejection forms.

## Suggested Next

- Execute 794 Step 3 only: write the exact handoff and obtain the required
  acceptance disposition for this one selected stack-restore row.

## Watchouts

- The handoff remains to 794, not a 734 receipt. `local_object_authority.live`
  is checkpoint-binding validity, not a per-VLA allocation lifetime state;
  retain the boundary against dynamic-VLA count, VLA GEP, other local/lifetime
  rows, Raw-BIR/importer/734 receipt, and presentation-derived facts.

## Proof

- Fresh Step 2 verification: `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  passed 1/1. The delegated packet prohibited canonical root-log changes, so
  the existing `test_after.log` remains owned by supervisor baseline policy.
