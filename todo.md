# Current Packet

Status: Active
Source Idea Path: ideas/open/801_bir_node_kind_tag_algebra_and_phase_vocabulary_lowering_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit the closed-746 descriptor and stage contract

## Just Finished

- Completed `plan.md` Step 1 as a bounded read-only audit.
- Current authority is `detail::node_kind_traits<K>` plus
  `node_kind_schema` in `src/backend/bir/core/ir.hpp:438-561`; it produces the
  compile-time descriptor/query surface at `ir.hpp:566-576` and runtime
  wrappers at `ir.hpp:578-664`. The foundation verifier consumes the shared
  payload and arity queries at `src/backend/bir/verify/verifier.cpp:64-66` and
  `:604-618`. Nearby compile-time/runtime, payload, arity, stage, and invalid
  kind coverage is `tests/backend/bir/backend_bir_node_kind_schema_test.cpp:20-94`.
- The exact normative delta for 801 is:
  - replace the flat descriptor fields (`family`, standalone `is_binary`,
    operand policy plus min/max, result policy, one effect enum, and raw byte
    stage mask at `ir.hpp:398-423`) with a closed, validated multi-axis tag
    algebra; `is_binary` is a one-off derived classification, operand
    fixed/variable is partly derivable from bounds, and `NodeFamily::Semantic`
    ambiguously excludes the still-semantic memory/call families;
  - derive stable compile-time and runtime queries from one authority. Today
    descriptor and payload dispatch each repeat the 16-kind switch inventory
    (`ir.hpp:578-601` and `:620-644`), while payload alternatives live only in
    the traits' `accepts` implementation and are absent from the descriptor;
  - define finite value/SSA, semantic-family, effect/control,
    stage-vocabulary, operand/result/type-policy, and MIR-realizability axes,
    including invalid combinations. The current schema has no SSA/non-SSA,
    SSA-eligible, no-value/value-producing, control/terminator, operand-role,
    type-policy, pseudo, machine, or MIR-ready term; `ResultArityPolicy` is only
    a count policy and `NodeEffect` is a single coarse memory/unknown value;
  - make stage admission exact. `NodeStage` has only Raw/Canonical/Prepared
    (`ir.hpp:408-412`), and every specialization uses `all_node_stages`
    (`ir.hpp:428-431,455-553`), so the current legality query proves no
    vocabulary transition, cannot reject a kind retained past its owner stage,
    and cannot describe pseudo/machine publication;
  - preserve the existing fail-closed runtime behavior: unknown enum values
    yield `nullopt`/`false` (`ir.hpp:578-664`), verifier payload/arity mismatch
    becomes `BoundedAlternative` failure (`verifier.cpp:604-618`), and tests
    exercise invalid value 255 (`backend_bir_node_kind_schema_test.cpp:66-94`).
    801 must additionally reject invalid tag combinations, illegal stage
    admission, descriptor/query drift, and unhandled transitions.
- Representative proof categories are bounded to existing semantic kinds
  (`Binary` as ordinary pure value, `Store` as zero-result memory effect, and
  `Phi` as the B4 SSA-boundary example) plus one contract-only category each
  for prepared, pseudo, and machine/MIR-ready classification. These latter
  categories are documentation/proof representatives, not a proposed full
  production kind list or phase-pass implementation.
- Exact artifact outline for
  `docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md`:
  1. authority, scope, and terminology;
  2. closed tag axes and composition/invalid-combination rules;
  3. `SsaEligible` versus B4 dynamic graph SSA proof;
  4. single hidden schema and stable compile-time/runtime query API;
  5. unknown-kind, illegal-stage, drift, and unhandled-case failure rules;
  6. B/C/D/E/F admitted vocabularies and transition matrix;
  7. retain/lower/expand/project/split/merge/disappear identity rules;
  8. publication-verifier obligations;
  9. bounded representative proof seam and adoption requirements for 732.
- Minimum optional C++ seam, deferred until Step 6 decides it is necessary:
  extend the existing hidden traits/descriptor authority with validated closed
  tags and derive paired compile-time/runtime queries across the bounded
  semantic plus contract-only prepared/pseudo/machine representatives; extend
  the nearby schema test for invalid combinations, unknown kinds, and illegal
  stages. Do not add phase passes, storage changes, or a full future enum.

## Suggested Next

- Execute only Step 2: define the closed tag algebra and the static
  `SsaEligible` versus dynamic B4 graph-SSA boundary in the normative artifact.

## Watchouts

- Do not revise or activate idea 732, reopen idea 746, or treat tag
  classification as proof of graph-stage SSA validity.
- `c4c-clang-tool` and `c4c-clang-tool-ccdb` were absent from `PATH`. The
  required `scripts/build_install_c4c_clang_tools.sh` retry failed because
  `/Users/chi-shengwu/c4c/ref/c4c-clang-tools` has no `CMakeLists.txt`; this
  audit therefore used bounded line-oriented inspection and did not repair
  tool infrastructure.

## Proof

- Passed: `git diff --check > test_after.log 2>&1`.
- Proof log: `test_after.log`.
