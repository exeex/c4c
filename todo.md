# Current Packet

Status: Active
Source Idea Path: ideas/open/718_prepared_routing_root_dependency_classification_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the blocked failure-family baseline

## Just Finished

- Step 1 reproduced the restored failure family without modifying dirty
  implementation or test candidates: 322/324 backend tests pass.
- `backend_aarch64_current_block_join_routing` fails first at MismatchedSource
  instruction 1 (`expected 0 actual 1`): the direct publication root must be
  rejected while its composed operand dependency remains authorized by
  semantic identity, not fixture numeric IDs.
- `backend_aarch64_instruction_dispatch` independently reaches the restored
  short-circuit row and reports one unsupported prepared BIR producer
  (`ops=1`, `diagnostics=1`). The MemorySource row is separately restored to
  `{false,true,false}` but is gated behind the earlier MismatchedSource failure
  in the same routing executable, so this baseline does not independently
  observe its result.
- Existing focused probe ownership maps to four seams under
  `tests/backend/case/`: direct publication identity, transitive dependency
  closure, stable-key composition, and no-policy/zero-fact behavior.

## Suggested Next

- Extract the next focused probe packet around direct-root rejection versus
  preserved composed dependency authority, using the existing authority-probe
  registration convention.

## Watchouts

- Do not alter or accept the dirty implementation/test candidates during the
  baseline packet.
- Preserve idea 716's unchanged policy-present vectors and original
  `%rhs.add`/`%short.selected` producer semantics.
- Do not encode values 810/811 or other fixture identity into authority rules.
- Probe executables are registered through
  `c4c_add_current_block_authority_probe` in `tests/backend/CMakeLists.txt`;
  each source lives under `tests/backend/case/`, links `c4c_backend`, uses C++17,
  registers with `add_test`, and receives `internal cpp authority_probe` labels.
- Candidate seams/files: `current_block_direct_publication_identity_probe.cpp`,
  `current_block_transitive_dependency_closure_probe.cpp`,
  `current_block_stable_key_composition_probe.cpp`, and
  `current_block_no_policy_zero_fact_probe.cpp`.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log 2>&1` exactly as delegated.
  Build passed; 322/324 tests passed. The two failures are
  `backend_aarch64_current_block_join_routing` and
  `backend_aarch64_instruction_dispatch`. Proof log: `test_after.log`.
