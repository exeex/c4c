# Current Packet

Status: Active
Source Idea Path: ideas/open/778_lir_logical_rhs_result_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish the logical RHS conversion result

## Just Finished

- Closed 779 capability-complete: its selected standalone-cast verifier/IR
  contract and focused positive/malformed proof are accepted; 778 now resumes
  at its recorded Step 1 clean producer reattempt.

## Suggested Next

- Dispatch Plan Step 1.

## Watchouts

- Do not reuse the former unaccepted `binary.cpp` diff. Limit the reattempt to
  the logical RHS non-`i1` conversion result; PHI result/incoming, generic
  expression APIs, and other producer families remain excluded.

## Proof

- 779 accepted proof: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
- 778 has no producer proof yet; run its focused proof after Steps 1–2.
