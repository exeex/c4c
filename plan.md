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

### Step 1 - Define native standalone cast result ownership

Goal: identify and implement the smallest verifier/IR rule that distinguishes a
result-producing standalone cast and requires a valid native result ID.

Primary targets:

- `src/codegen/lir/verify.cpp`
- LIR result ownership collection definitions, only if required by the rule

Actions:

- retain existing cast kind and endpoint-type validation;
- require result authority only for the bounded standalone result-producing
  cast contract, without altering PHI or logical producer lowering;
- ensure invalid, duplicate, and foreign IDs are checked through the existing
  ownership model rather than a parallel lookup.

Completion check:

- the verifier has one native fail-closed path for all four malformed
  standalone cast-result cases.

### Step 2 - Add focused positive and malformed cast coverage

Goal: prove the standalone cast contract structurally and exercise missing,
invalid, duplicate, and foreign result authority.

Primary target:

- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

Actions:

- use a standalone cast fixture and native IDs, not rendered-result matching;
- preserve a valid positive cast case;
- add one malformed mutation for each required failure family;
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
