# LIR Standalone Cast Result Authority Contract Runbook

Status: Active
Source Idea: ideas/open/779_lir_cast_result_authority_contract.md
Supersedes: `ideas/open/778_lir_logical_rhs_result_authority_publication.md` while its verifier/IR prerequisite is active.

## Goal

Establish the minimal verifier/IR contract that makes malformed standalone
`LirCastOp.result` authority fail closed.

## Core Rule

Use native `LirValueId` ownership only. A cast result must never derive
authority from rendered text, instruction order, or a testcase-specific path.

## Read First

- `ideas/open/779_lir_cast_result_authority_contract.md`
- `ideas/open/778_lir_logical_rhs_result_authority_publication.md`
- `src/codegen/lir/verify.cpp`
- the LIR operand/result definitions and result-ownership collection path
- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

## Non-Goals

- Do not edit `emit_logical` or accept/reuse 778's unaccepted `binary.cpp`
  change.
- Do not change PHI result/incoming representation or verification.
- Do not migrate generic expression APIs or other producer families.

## Ordered Steps

### Step 1 - Repair native standalone cast result ownership

Goal: complete the smallest verifier/IR rule that distinguishes a
result-producing standalone cast, requires a valid native result ID, and
rejects an ID whose defining result belongs to another function.

Primary targets:

- `src/codegen/lir/verify.cpp`
- LIR result ownership collection definitions, only if required by the rule

Actions:

- retain the partial Step 1 commit `5a9888938`: its explicit standalone-cast
  selection and missing-result rejection are accepted but incomplete;
- add native definition provenance or an equivalent module-level ownership
  check so `verify_function_value_ownership` can distinguish a fresh local
  definition from a `LirValueId` already defined as a result in another
  `LirFunction`;
- require selected standalone casts to reject cross-function result reuse
  while retaining existing invalid-ID and same-function duplicate behavior;
- retain existing cast kind and endpoint-type validation;
- keep the ownership rule native and bounded: do not alter PHI or logical
  producer lowering, and do not introduce rendered-text, result-name-map,
  side-table, or testcase-specific authority.

Completion check:

- the verifier has a native fail-closed path for missing, invalid, duplicate,
  and cross-function foreign standalone cast-result IDs; a foreign ID cannot
  be accepted merely because the current function has not yet defined it.

### Step 2 - Add focused positive and malformed cast coverage

Goal: prove the standalone cast contract structurally and exercise missing,
invalid, duplicate, and foreign result authority.

Primary target:

- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

Actions:

- use a standalone cast fixture and native IDs, not rendered-result matching;
- preserve a valid positive cast case;
- add one malformed mutation for each required failure family only after Step
  1 proves cross-function provenance: missing, invalid, same-function
  duplicate, and foreign reuse from a different `LirFunction`;
- keep PHI and logical-RHS fixtures out of this packet.

Completion check:

- focused tests demonstrate every malformed case is rejected by the verifier.

### Step 3 - Validate and publish the 778 handoff

Goal: validate the bounded contract and document the precise parent return
point.

Actions:

- build and run the focused frontend-LIR test;
- record the accepted verifier/IR contract and proof in the execution state;
- hand 778 back to Step 1 for a clean producer reattempt, without accepting its
  prior local diff.

Completion check:

- 778 can resume from its recorded Step 1 with this verifier contract available,
  while PHI and generic logical work remain separate.

## Proof

- For code steps: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
- The supervisor owns baseline/regression logs and final acceptance.
