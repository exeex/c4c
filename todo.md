# Current Packet

Status: Active
Source Idea Path: ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the existing `ull` DirectScalar authority seam

## Just Finished

- Lifecycle switch from 734 Step 7.35: its current receiver work is
  unaccepted; accepted 734 history remains through Step 7.34 (`8418036b1`).

## Suggested Next

- Trace the existing `ull` native DirectScalar producer/emitter path and the
  corresponding `native_body_parameter_definitions` verifier inputs.

## Watchouts

- Stop if repairing `ull` requires broadening to a DirectScalar authority
  family; create no such expansion in this blocker.
- Do not touch Raw-BIR/importer code or use presentation text as authority.

## Proof

- Before acceptance: fresh build, focused producer/verifier proof, exact
  `^llvm_gcc_c_torture_src_20041011_1_c$` check, then supervisor-selected
  matching regression/broader proof.
