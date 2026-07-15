# Current Packet

Status: Step 1 complete
Source Idea Path: ideas/open/761_lir_call_signature_type_mirror_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map selected call/signature authority seams

## Just Finished

- Step 1: made complete `LirCallOp.structured_args` plus matching
  `arg_type_refs` authoritative in the shared typed-call helpers, rendering
  argument and fixed-signature types from `LirTypeRef` rather than their text
  mirrors; incomplete mirrors still use the raw `args_str` compatibility parser.
  Added focused backend coverage where stale argument, suffix, and signature
  type text cannot override the structured `i32 %actual` facts.

## Suggested Next

- Select the next bounded Step 1 call/signature authority seam; do not widen
  this completed call-argument helper packet into selector or inline-assembly work.

## Watchouts

- Do not treat `args_str`, fixed parameter type text, selector text, or inline
  assembly template/constraint text as semantic authority when structured
  facts exist; retain explicit raw fallbacks where structured data is absent.
- Empty or partial structured argument mirrors intentionally remain on the raw
  compatibility path because they cannot prove complete call-argument authority.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log` — passed (5/5 backend tests); proof log: `test_after.log`.
