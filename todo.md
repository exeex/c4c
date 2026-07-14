# Current Packet

Status: Active
Source Idea Path: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish and prove production computed-goto address carrier authority

## Just Finished

- Switched from concluded 765 after its accepted Step 1 (`1e24e2081`) repaired
  the first upstream producer seam: `emit_member_rval_operand` now carries the
  final `emit_bitfield_load` current-function `LirValueId` for
  `insn.f1.offset`. The full baseline candidate must not be accepted yet:
  `comp-goto-1` and four same-family consumers now fail at the downstream
  `LirIndirectBrOp.addr_value` carrier check.

## Suggested Next

- Reproduce the five affected tests, then complete 764 Step 1 by publishing
  the verifier-valid pointer identity from the now-authoritative computed-goto
  address GEP into `LirIndirectBrOp.addr_value`. Do not resume 734 directly.

## Watchouts

- Do not change Raw-BIR/importer or re-execute 734 Step 7.24. Do not weaken
  `verify_authoritative_gep`, synthesize a GEP ID from a partial/raw index, or
  derive authority from rendered operands, labels, LLVM/printer text, or a
  testcase name.
- Treat `comp-goto-1`, `20040302-1`, `20041214-1`, `920501-4`, and `920501-5`
  as one downstream consumer family for repair/proof; none may be excluded,
  downgraded, or accepted as a baseline exception.

## Proof

- 765 accepted proof: `1e24e2081`; fresh `cmake --build --preset default`;
  `^frontend_lir_call_type_ref$` passed; matching subset guard passed with
  the known downstream comp-goto failure unchanged; broad backend guard
  passed.
- Baseline guard is rejected: baseline `c8a205218` was 3034/3034; candidate
  `1e24e2081` has five `LirIndirectBrOp.addr_value` current-function-pointer
  failures. Before any future baseline accept, run the same five affected
  tests after the carrier repair and then the supervisor-selected full
  candidate; require no new baseline failures.
