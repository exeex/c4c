# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Complete ordinary instruction semantic families

## Just Finished

- Completed Plan Step 4.5.3 by preserving native `LirFunction::is_internal`
  and `can_elide_if_unreferenced` through typed FunctionData, builder creation,
  immutable Raw/Canonical views, reachable verification, and LIR import.
- Enforced exactly the producer-valid false/false declaration and definition,
  false/true helper definition, and true/true static definition combinations;
  invalid metadata and conflicting merges reject before mutation.
- Proved a production static literal-return function retained by a structured
  global function-pointer initializer reaches semantic BIR with its ordinary
  external neighbor; no global-row blocker remained.

## Suggested Next

- Begin Plan Step 5 by selecting one bounded ordinary-instruction family with
  complete typed operand/result authority and an exact builder/view/verifier/
  importer proof contract.

## Watchouts

- Function linkage/elision facts remain native booleans; do not infer them from
  names, signature rendering, body presence, source order, or testcase
  identity, and do not OR/overwrite merge conflicts.
- Keep CFG, block targets/edges, stack slots, allocas, local objects, body
  parameter binding, and lifetime state outside this packet.
- Keep `long` and `unsigned long` fail-closed pending inactive idea 743; do not
  change I686 width semantics inside idea 734.
- Signature `ParameterDef` ordinals are BIR storage order only. Body parameter
  use still lacks native source identity and must not be reconstructed from
  names, raw operands, signature text, or ABI position.
- Production `param_slot.c` now passes signature receipt and rejects later at
  `UnsupportedAllocaInstructions`; pointer parameters still reject at stable
  `UnsupportedFunctionParameters`.

## Proof

- Fresh `cmake --build --preset default` completed successfully.
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$'
  --output-on-failure` passed 1/1 with builder, verifier, Raw/Canonical view,
  misleading-display, merge-conflict, and module-rollback coverage.
- `c4cll --dump-bir` published the retained static literal-return production
  function; the neighboring LLVM observation confirmed its structured global
  function-pointer initializer kept it reachable and was not used as metadata
  authority.
- `ctest --test-dir build -j --output-on-failure > test_after.log` passed
  3033/3033. The monotonic guard against `test_before.log` passed with delta
  `passed=0 failed=0` and no new over-30-second tests.
