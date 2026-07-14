# Current Packet

Status: Active
Source Idea Path: ideas/open/772_lir_gep_pointer_authority_pr70460.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and repair the production GEP pointer authority loss

## Just Finished

- 773 is closed as capability complete: `a4415f99c` accepted verifier
  admission, `c64b78c48`/`97121c359` supplied typed Raw-BIR authority, and
  `0d0f0725b` completed table-backed printer and typed lowering. Its fresh
  build, exact `^backend_` proof (5/5), and matching monotonic guard are
  accepted. 772 now resumes at its preserved Step 1 forwarding repair.

## Suggested Next

- Executor: repair only the structured `emit_indexed_gep` direct-constant
  forwarding seam, add nearby production and malformed-authority coverage,
  fresh-build, and make `llvm_gcc_c_torture_src_pr70460_c` pass. Do not
  revisit 773 contracts or evaluate a new baseline first.

## Watchouts

- Use the accepted table-backed typed direct-label-address authority only. Do
  not recover labels from text, fabricate SSA/globals, coerce to a global base,
  widen GEP authority, or edit baseline artifacts. The rejected pr70460
  baseline remains rejected until this step passes.

## Proof

- Required for Step 1: fresh `cmake --build --preset default`, focused
  `llvm_gcc_c_torture_src_pr70460_c` proof, and nearby production/malformed
  authority coverage. The supervisor, not this packet, selects and evaluates
  any subsequent full-suite candidate.
