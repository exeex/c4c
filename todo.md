# Current Packet

Status: Active
Source Idea Path: ideas/open/801_bir_node_kind_tag_algebra_and_phase_vocabulary_lowering_contract.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Land the bounded schema/query proof if required

## Just Finished

- Completed `plan.md` Step 7 as the bounded C++17 convergence proof.
- `NodeKind::Count` and one hidden ordered constexpr registry now provide the
  complete current 16-kind inventory. Each entry owns the legacy-compatible
  descriptor projection, bounded value/SSA/family/refinement/effect/control/
  stage/shape/type/payload/MIR facts, and its closed payload predicate.
- Namespace-scope constexpr validation proves registry size/order/completeness,
  non-null payload mechanics, known axis values, arity/result/type/SSA
  relations, refinement constraints, explicit non-empty stage admission/owner,
  family-stage rules, and MIR constraints before queries instantiate.
- Compile-time schema/tag/admission and stage-qualified SSA helpers plus runtime
  wrappers derive from that registry. Named value/memory/call/terminator/read/
  write/trap/expansion/allocation/machine helpers are derived facts and do not
  claim B4 graph validity.
- Removed both repeated 16-kind runtime switch inventories. Descriptor lookup,
  payload acceptance, arity checks, and classification now consume one entry.
  Current kinds explicitly admit Raw/Canonical and immutable-reference
  Prepared, while rejecting PseudoPreallocation/Allocated/MirReadyMachine.
- Extended the nearby schema test with compile/runtime agreement for
  `Binary`, `Store`, and `Phi`; stage admission/rejection; unknown kind/tag/
  stage; payload/arity mismatch; inventory validation; invalid combinations;
  and non-production constexpr Prepared/Pseudo/Machine fixture schemas. No
  speculative production kinds or phase/pass/storage code was added.

## Suggested Next

- Execute only Step 8: map final artifact/code/test evidence to every source
  criterion and hand it to plan-owner for the explicit close decision.

## Watchouts

- Do not revise or activate idea 732, reopen idea 746, or treat tag
  classification as proof of graph-stage SSA validity.
- The complete future B-F production vocabulary remains intentionally absent;
  later phase work must add kinds only through the reviewed registry contract.

## Proof

- Passed: `cmake --preset default -DENABLE_C4C_BACKEND=ON && cmake --build
  --preset default && ctest --test-dir build -j --output-on-failure -R
  '^backend_' > test_after.log`.
- Backend subset: 6/6 passed, including `backend_bir_node_kind_schema`.
- Proof log: `test_after.log`.
