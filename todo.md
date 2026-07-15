# Current Packet

Status: Active
Source Idea Path: ideas/open/746_bir_node_kind_centric_storage_pass_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Establish the NodeKind schema and helper API

## Just Finished

- Completed `plan.md` Step 2 as one bounded schema/helper slice. `NodeKind` is
  now the durable public name for the closed 16-kind BIR vocabulary, with
  `Opcode` retained only as a compatibility alias.
- Added one hidden C++ traits authority and public compile-time/runtime queries
  for family and binary classification, operand/result arity policy, accepted
  payload alternatives, effects, and legal stage masks. Runtime queries reject
  invalid/unhandled kinds, and concrete `Type` remains outside kind traits.
- Replaced verifier-local opcode/payload duplication with
  `node_kind_accepts_payload`; builders, instruction/result storage, value
  representation, and documentation remain unchanged.
- Added nearby multi-kind coverage for semantic, memory, call, and authority
  descriptors, alternate accepted payloads, mismatched payloads, compile-time
  queries, stage legality, and invalid runtime kinds.

## Suggested Next

- Execute only the next `plan.md` Step 3 consumer/storage-alignment packet:
  choose one bounded repeated verifier classification or arity consumer and
  migrate it to the shared NodeKind authority while retaining the current
  `InstData` and value/result compatibility storage.

## Watchouts

- `InlineAsm` and `Call` retain conservative `Many` result and `Unknown`
  effect policies because payload facts refine their concrete behavior; do not
  infer concrete result `Type` or call/asm effects from kind alone.
- All current kinds are legal across Raw, Canonical, and Prepared stages. The
  stage mask is intentionally explicit so later passes can narrow legality in
  the same traits authority instead of creating pass-local tables.
- `GetElementPtr`, `Load`, and authority variants still share public kinds with
  differing current operand shapes. Their descriptors therefore preserve the
  present compatibility ranges rather than claiming a storage normalization
  that Step 2 did not perform.

## Proof

- Supervisor-selected proof:
  `cmake --preset default -DENABLE_C4C_BACKEND=ON && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.
- Result: passed; 6/6 matching backend tests passed, including
  `backend_bir_node_kind_schema`; log path `test_after.log`.
- `git diff --check` also passed after the proof.
