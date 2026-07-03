Status: Active
Source Idea Path: ideas/open/562_bir_direct_call_semantic_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce Direct-Call Boundary Evidence

# Current Packet

## Just Finished

Step 1: reproduced retained representative
`tests/c/external/gcc_torture/src/20000717-1.c` through the semantic BIR
boundary and captured same-snapshot evidence under
`build/agent_state/562_step1_direct_call_boundary/`.

Exact commands:
- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000717-1.c > build/agent_state/562_step1_direct_call_boundary/20000717-1.dump-bir.stdout 2> build/agent_state/562_step1_direct_call_boundary/20000717-1.dump-bir.stderr`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000717-1.c > build/agent_state/562_step1_direct_call_boundary/20000717-1.dump-prepared-bir.stdout 2> build/agent_state/562_step1_direct_call_boundary/20000717-1.dump-prepared-bir.stderr`
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000717-1.c -o build/agent_state/562_step1_direct_call_boundary/20000717-1.rv64.o > build/agent_state/562_step1_direct_call_boundary/20000717-1.codegen-obj.stdout 2> build/agent_state/562_step1_direct_call_boundary/20000717-1.codegen-obj.stderr`
- `./build/c4cll --codegen llvm --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000717-1.c -o build/agent_state/562_step1_direct_call_boundary/20000717-1.llvm.ll`

Diagnostic text from semantic BIR:
`error: backend BIR dump requires semantic lir_to_bir lowering before the prepared handoff: semantic lir_to_bir failed outside the currently admitted capability buckets covering function-signature, scalar-control-flow, scalar/local-memory (including scalar-cast/scalar-binop and alloca/gep/load/store local-memory), and local/global memory semantics, plus semantic call families (direct-call, indirect-call, and call-return) and explicit runtime or intrinsic families such as variadic, stack-state, absolute-value, memcpy, memset, and inline-asm placeholders; latest function failure: semantic lir_to_bir function 'foo' failed in semantic call family 'direct-call semantic family'`

Failing call tied to the row:
- Source call: `foo` returns `bar(i, t)` in
  `tests/c/external/gcc_torture/src/20000717-1.c`.
- Auxiliary LLVM route names the matching call as
  `%t0 = call i32 (i32, %struct.trio) @bar(i32 %p.i, %struct.trio %p.t)` in
  `build/agent_state/562_step1_direct_call_boundary/20000717-1.llvm-call-sites.txt`.
- Retained direct-call family row:
  `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md` lists
  `src/20000717-1.c`, function `foo`, topic
  `semantic call family 'direct-call semantic family'`.

Evidence locations and route boundary:
- Semantic BIR evidence:
  `build/agent_state/562_step1_direct_call_boundary/20000717-1.dump-bir.stderr`
  with status `exit=1`; no BIR body was emitted.
- Prepared BIR rejection:
  `build/agent_state/562_step1_direct_call_boundary/20000717-1.dump-prepared-bir.stderr`
  with status `exit=1`; it rejects before prepared handoff because semantic
  `lir_to_bir` failed in the same `foo` direct-call family.
- RV64 object route rejection:
  `build/agent_state/562_step1_direct_call_boundary/20000717-1.codegen-obj.stderr`
  with status `exit=2`; object handoff rejects for the same semantic
  `lir_to_bir` prerequisite failure.
- One failing call was tied to a visible semantic BIR fact family: yes, the
  only call in `foo` is the direct call to `bar`, and the semantic boundary
  reports `foo` in `direct-call semantic family`. The exact first bad
  direct-call fact is not yet classified.

## Suggested Next

Continue with Step 2: inspect call-publication evidence for the `foo` to `bar`
direct call in `src/20000717-1.c` and classify the first bad fact as callee,
argument source, return result, byval/sret, prepared handoff, or evidence gap.

## Watchouts

- The representative fails before a semantic BIR body is emitted, so prepared
  BIR and RV64 object evidence are rejection points, not later-stage ownership
  proof.
- The call passes a `struct trio` argument through a direct call; byval/sret or
  aggregate argument publication may be relevant, but Step 1 did not prove the
  first bad fact.
- Do not use expectation rewrites, unsupported downgrades, diagnostic renames,
  or named-case shortcuts as progress.

## Proof

Ran delegated proof:
`git diff --check -- todo.md && scripts/plan_review_state.py show`.

Result: passed. `scripts/plan_review_state.py show` reported
`current_step_id` as `1`, `current_step_title` as
`Reproduce Direct-Call Boundary Evidence`, and no pending baseline or code
review. This evidence-only proof did not create or update `test_after.log`.
