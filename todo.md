Status: Active
Source Idea Path: ideas/open/562_bir_direct_call_semantic_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Missing Direct-Call Fact

# Current Packet

## Just Finished

Step 2: classified the first missing direct-call fact for retained
`tests/c/external/gcc_torture/src/20000717-1.c` as **byval aggregate argument
handoff**, not callee classification, return result publication, argument
source publication, or prepared handoff.

Same-run evidence:
- Step 1 semantic BIR still fails before any BIR body is emitted:
  `build/agent_state/562_step1_direct_call_boundary/20000717-1.dump-bir.stderr`
  reports `semantic lir_to_bir function 'foo' failed in semantic call family
  'direct-call semantic family'`.
- Step 1 prepared/object evidence rejects before prepared handoff for the same
  semantic prerequisite failure:
  `build/agent_state/562_step1_direct_call_boundary/20000717-1.dump-prepared-bir.stderr`
  and
  `build/agent_state/562_step1_direct_call_boundary/20000717-1.codegen-obj.stderr`.
- The matching LLVM call is
  `%t0 = call i32 (i32, %struct.trio) @bar(i32 %p.i, %struct.trio %p.t)` in
  `build/agent_state/562_step1_direct_call_boundary/20000717-1.llvm-call-sites.txt`.
- Focused HIR evidence captured in
  `build/agent_state/562_step2_direct_call_fact/20000717-1.dump-hir.stdout`
  shows `struct trio size=12 align=4` and `foo` as `return bar(i#P1, t#P0)`.
- Focused debugger evidence captured in
  `build/agent_state/562_step2_direct_call_fact/20000717-1.gdb-note.stdout`
  confirms the failure note is emitted from
  `BirFunctionLowerer::lower_call_inst` before `lower_block` returns.

Classification:
- Direct callee classification is not the first bad fact: Step 1 and LLVM name
  the call as direct `@bar`, and the semantic diagnostic reaches the
  direct-call family in `foo`.
- Return result publication is not the first bad fact: the call return is scalar
  `i32`, with no sret aggregate return in the source, HIR, or LLVM call.
- Prepared handoff is not the first bad fact: both prepared BIR and RV64 object
  paths reject because semantic `lir_to_bir` never publishes the call.
- Route6 argument-source/publication records are unavailable, not negative
  evidence, because no semantic BIR `CallInst` is emitted for this call.
- The first visible non-scalar fact is the by-value `struct trio` argument
  `t#P0`/`%p.t`. `lower_call_inst` has a dedicated byval aggregate path that
  must lower a byval aggregate operand to a pointer-backed BIR call argument
  before `CallArgumentSourceRelationship`/route6 publication can exist.
  Therefore the first owner is BIR byval aggregate argument handling for a
  direct-call argument sourced from a by-value aggregate parameter.

## Suggested Next

Continue with Step 3: add focused BIR call-publication coverage for a direct
call that passes a by-value aggregate parameter as an aggregate argument. The
coverage should expose the expected byval ABI/source fact before any repair is
attempted.

## Watchouts

- The representative fails before a semantic BIR body is emitted, so prepared
  BIR, RV64 object, and route6 lookup evidence are absence-of-publication
  boundaries, not later-stage ownership proof.
- Keep the next packet independent of `src/20000717-1.c` if practical: the
  missing fact is semantic byval aggregate argument handoff for direct calls,
  not a named-case shortcut.
- Do not use expectation rewrites, unsupported downgrades, diagnostic renames,
  or named-case shortcuts as progress.

## Proof

Ran delegated proof:
`git diff --check -- todo.md && scripts/plan_review_state.py show`.

Result: passed. `scripts/plan_review_state.py show` reported
`current_step_id` as `2`, `current_step_title` as
`Classify The Missing Direct-Call Fact`, and no pending baseline or code
review. This evidence-only packet did not create or update `test_after.log`.
