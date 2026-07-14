# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.32
Current Step Title: Blocked — direct external double call-result authority

## Just Finished

- Plan Step 7.32 is blocked before a verifier change: the production unresolved
  direct-callee route does emit `extern_decl_link_name_map` and `extern_decls`,
  but does not retain a `FnPtrSig` for a plain `DeclRef`. Its zero-argument
  external `double` call therefore has no structured fixed-empty signature and
  does not allocate the required `fresh_value(ctx)` result ID.

## Suggested Next

- Do not retry Step 7.32 as a verifier packet. It depends on the separate,
  inactive producer initiative
  `ideas/open/746_lir_unresolved_external_direct_call_signature_result_authority.md`.
  Resume only when that initiative proves a source-level unresolved-external
  route with a retained native `FnPtrSig`, structured fixed-empty signature,
  and a `fresh_value(ctx)` call-result `LirValueId`.

## Watchouts

- Do not fabricate the positive path by mutating HIR authority or moving a
  local prototype into external declaration rows. The missing `FnPtrSig`/
  fresh-result producer seam is outside this packet's owned files; no verifier
  relaxation may turn text-only results into authority.

## Proof

- `cmake --build --preset default && ctest --test-dir build -R
  '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log 2>&1`
  passed after reverting the scoped exploratory changes; `git diff --check`
  passed. Proof log: `test_after.log`.
