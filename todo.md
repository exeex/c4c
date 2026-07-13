# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.1
Current Step Title: Receive direct zero-argument void calls

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
- Supervisor authority audit selected only the modern direct zero-argument
  void `LirCallOp` row for the first bounded Step 5 packet.

## Suggested Next

- Execute Plan Step 5.1 by adding a zero-operand/zero-result BIR `Call` node,
  exact `LinkNameId -> FunctionId` resolution, two-pass/equivalent forward
  target safety, verifier coverage, and transactional proof.

## Watchouts

- Call identity comes only from native `direct_callee_link_name_id`; `callee`,
  names, `args_str`, and `callee_type_suffix` are presentation only.
- Require a present structured void, zero-fixed, nonvariadic, specified callee
  signature that agrees with the resolved module function; do not parse text to
  fill missing or conflicting facts.
- Function creation must precede or otherwise safely support body target
  resolution so source order, forward calls, and valid recursion do not change
  semantics.
- Keep nonvoid/result calls, all arguments, indirect calls, variadic or
  unspecified signatures, CFG/local/body binding, and every other ordinary
  instruction row fail-closed.
- Preserve existing function linkage/elision merge rules and keep `long`/
  `unsigned long` blocked pending inactive idea 743.

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
