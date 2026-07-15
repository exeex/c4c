# Current Packet

Status: Active
Source Idea Path: ideas/open/746_bir_node_kind_centric_storage_pass_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Align core storage and one bounded pass/query consumer

## Just Finished

- Completed `plan.md` Step 3 as one bounded verifier/storage-contract slice.
  Added the public fail-closed `node_kind_accepts_arity` runtime helper over the
  existing schema's fixed, bounded-variable, zero, one, and many policies.
- FoundationVerifier now applies that helper as a generic operand/result-count
  precondition beside payload acceptance. Existing payload-specific semantic
  checks remain intact and continue to provide their stricter diagnostics.
- Clarified in core declarations that operands are ordered input uses; results
  and `InstResultDef::result_index` are bootstrap compatibility storage;
  concrete type belongs to `ValueDef`; and the function arena/`InstId`, rather
  than `InstData`, owns instruction identity. No field or layout changed.
- Extended the nearby schema test across fixed zero-result, fixed one-result,
  bounded-variable, many-result, and invalid-kind arity cases. The full backend
  fixture set also exercised the verifier gate over builder-produced BIR.

## Suggested Next

- Execute only `plan.md` Step 4 documentation: update the owned BIR core
  contract documentation to describe NodeKind traits, fail-closed pass queries,
  and the explicitly retained bootstrap storage boundaries.

## Watchouts

- The generic gate deliberately checks only kind-level count policy. Payload
  semantics such as Phi incoming parity, call signatures, and concrete value
  types remain in their existing exact verifier checks.
- `Many` currently means an open result-count policy, not a promise that every
  count is constructible for every payload. `InlineAsm` and `Call` still need
  payload/value facts to refine their concrete result contract.
- No private malformed-`InstData` test seam exists in the focused schema test;
  direct helper rejection is covered nearby, while the six backend tests prove
  the verifier accepts current valid storage. Do not expose private storage
  merely to manufacture a negative verifier fixture.

## Proof

- Supervisor-selected proof:
  `cmake --preset default -DENABLE_C4C_BACKEND=ON && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.
- Result: passed; 6/6 matching backend tests passed, including
  `backend_bir_node_kind_schema`; log path `test_after.log`.
- `git diff --check` also passed after the proof.
