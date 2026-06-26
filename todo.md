Status: Active
Source Idea Path: ideas/open/375_rv64_object_route_scalar_compare_trunc_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Scalar Compare Trunc Facts

# Current Packet

## Just Finished

Completed Plan Step 1 audit for the `showbug` scalar compare/trunc shape in
`tests/c/external/gcc_torture/src/20000217-1.c`.

Concrete prepared shape for the implementation packet:

- Semantic BIR has `%t9 = bir.sge i32 %t8, 8`, then
  `%t11 = bir.trunc i1 %t9 to i16`, then `bir.ret i16 %t11`.
- Prepared BIR promotes the compare result to target-facing integer form:
  `%t9 = bir.sge i32 %t8, 8`, `%t11 = bir.trunc i32 %t9 to i16`, then
  `bir.ret i16 %t11`.
- `%t8` is a GPR-loadable prepared value in `t0`; `%t9` is the named compare
  result in GPR home `s1`; `%t11` is an `i16` stack-slot home at slot `#11`,
  stack offset `20`.
- The publication chain is `before_instruction` at instruction index `11`
  moving compare result `%t9` to trunc destination `%t11`, then
  `before_return` moving `%t11` to return ABI register `a0`.
- Existing RV64 object compare support records `PreparedObjectCompare` facts
  and consumes them for conditional branches; this packet identifies the new
  needed consumer as compare-result materialization for a named compare feeding
  integer trunc publication.
- First supportable lowering should admit only prepared scalar integer
  compare-result materialization where the trunc source is exactly one known
  named compare result, the predicate is `Sge`, compare width is `i32`, the RHS
  immediate is materializable, the destination trunc width is `i16`, and the
  destination has a supported prepared stack-slot/register publication path.

Audit artifact: `build/agent_state/375_step1_scalar_compare_trunc_audit.txt`.

## Suggested Next

Execute Plan Step 2 by adding the focused RV64 object-emission lowering path
and tests for the audited prepared compare-result trunc publication shape.

## Watchouts

- Do not reopen frame-slot address call-argument materialization, pointer-value
  local memory, or frame-slot payload-value call arguments; ideas 372, 368, and
  373 already own those routes.
- Do not absorb non-register parameter homes, aggregate `va_arg`, byval homes,
  terminators, CFG reconstruction, or source-shape shortcuts.
- Progress must come from prepared compare/trunc metadata, not testcase names,
  hard-coded constants, value ids, frame slots, or registers.
- Keep unsupported adjacent shapes fail-closed: predicates not deliberately
  admitted by Step 2, compare widths other than the audited `i32`, trunc
  destinations other than the audited integer `i16`, trunc sources that are not
  exactly one known named prepared compare result, missing/dynamic/ambiguous
  homes, FPR/vector/aggregate/non-integer destinations, and unrelated branch,
  select, call, memory, address, terminator, or CFG routes.
- The source path named in the packet as `src/20000217-1.c` exists in this
  checkout as `tests/c/external/gcc_torture/src/20000217-1.c`.

## Proof

Audit-only packet. No build proof required and no root-level proof log was
written. Audit inputs were read from
`build/agent_state/372_step2_20000217-1.showbug-semantic-bir.txt` and
`build/agent_state/372_step2_20000217-1.showbug-prepared-bir.txt`; the Step 1
summary artifact is
`build/agent_state/375_step1_scalar_compare_trunc_audit.txt`.
