Status: Active
Source Idea Path: ideas/open/602_bir_local_memory_load_semantics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Representative Load-Family Proof Rows

# Current Packet

## Just Finished

Activated `ideas/open/602_bir_local_memory_load_semantics.md` into `plan.md`.
No implementation, tests, expectations, unsupported markers, allowlists,
runtime/timeout/accounting behavior, other open ideas, or closed ideas were
touched.

## Suggested Next

Execute Step 1: select multiple representative RV64 gcc_torture backend-object
rows from the local-memory load semantic family, select adjacent guard rows, and
record the proof command before implementation starts.

## Watchouts

- Keep named cases as probes only, not match keys.
- Do not route this through RV64 target inference, expectation edits,
  unsupported markers, allowlists, timeout/runtime handling, or accounting.
- Escalate for plan review if the evidence points to store, GEP, alloca,
  prepared authority, RV64 consumer, ABI, or runtime ownership instead of BIR
  load production.

## Proof

Lifecycle activation only. No build or test proof required.
