Status: Active
Source Idea Path: ideas/open/656_20000722_local_memory_access_object_route.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Object-Route Local-Memory Evidence

# Current Packet

## Just Finished

Completed Step 1 for idea 656 by refreshing focused diagnostics for
`tests/c/external/gcc_torture/src/20000722-1.c`.

Evidence is under
`build/agent_state/656_step1_local_memory_access_evidence/`, with summary in
`build/agent_state/656_step1_local_memory_access_evidence/summary.md`.

The first unsupported local-memory access owner is `bar:entry` instruction 7:
`@.str0 = bir.load_local ptr %lv._clit_.0, addr .str0`. Prepared addressing
classifies it as a string-constant label-pointer local load:
`base=string_constant`, `result=@.str0`, `symbol=.str0`, `offset=0`,
`size=8`, `align=8`, `base_plus_offset=yes`,
`layout_authority=string_constant_label_pointer`, and
`range_verdict=unknown_compatible`.

The RV64 object route rejects before object emission with
`unsupported_local_memory_access: RV64 object route requires prepared frame-slot
or pointer-value base-plus-offset local memory addressing`. The diagnostic is
produced by the `LoadLocalInst` local-memory support predicate in
`src/backend/mir/riscv/codegen/object_emission.cpp`, through the prepared
string-constant label-pointer authority check shared with prepared local-memory
emission.

The later `bar:entry` instruction 10 call-argument frame-slot materialization is
present in prepared BIR, but the object route fails on instruction 7 before any
object-emission call-argument decision is reachable. Isolating probes confirm
the compound-literal string-label pointer shape fails with the same diagnostic,
while the later `foo` pointer-value `p->t` load/store shape succeeds in RV64
object mode.

## Suggested Next

Execute Step 2 for idea 656: locate why the RV64 object-route
`LoadLocalInst` predicate rejects the prepared string-constant label-pointer
access at `bar:entry` instruction 7, then decide whether this is a narrow
regression in the closed idea 630/638 authority path or requires lifecycle
review before implementation.

## Watchouts

- Do not change idea 648 call-argument materialization in this route.
- Do not rely on the historical `mv a0,s2` disassembly as current evidence.
- Do not broaden string-constant local-memory policy or
  `StringConstantLabelPointer` admission.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.
- Do not treat the later `foo` pointer-value access as the first blocker; the
  focused `probe_pointer_value.c` object route succeeds.
- If diagnostics reach a renewed call-argument mismatch after this blocker,
  hand that back to idea 648 instead of fixing it here.

## Proof

No build or ctest proof was required for this evidence-only packet. Ran focused
prepared-BIR, BIR, MIR summary, MIR trace, RV64 asm-route, RV64 object-route,
and two local diagnostic probes for
`tests/c/external/gcc_torture/src/20000722-1.c`. Did not create or overwrite
`test_after.log`.
