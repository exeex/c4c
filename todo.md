Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Validation And Handoff To Idea 105

# Current Packet

## Just Finished

Completed `plan.md` Step 6 by running the supervisor-selected final validation
and recording the handoff classification for the HIR safe legacy lookup
demotion runbook. No implementation file changed in this packet.

Final runbook classification:
- Safe HIR-internal demotions from this runbook are complete for the selected
  review-backed scope: complete-`DeclRef` HIR lookup paths prefer structured
  lookup, direct method/member probes touched by the runbook route through
  parity-recording helpers, and owner-aware HIR record checks use
  `find_struct_def_by_owner_structured` when valid owner metadata is available.
- No remaining idea 104 work is explicitly blocked inside the HIR-only scope.
  The remaining retained legacy paths are not deletion-ready under this
  runbook because they are classified as `bridge-required`,
  `legacy-proof-only`, or `needs-more-parity-proof`.
- Retained proof-only instrumentation includes decl lookup hit/parity records,
  owner/rendered struct-def parity mismatches, and Lowerer method/member parity
  counters. These remain observation/proof infrastructure rather than final
  semantic authority removal targets for this packet.
- Bridge-required work is separated for idea 105: HIR-to-LIR string identity,
  `TypeSpec::tag` codegen/layout consumers, extern filtering, global
  best-object selection, rendered `Module::struct_defs` layout/base/member
  paths, rendered method/static-member maps, ABI/link spelling, and downstream
  fallback-name handoff.

Handoff status:
The active runbook is ready for supervisor and plan-owner lifecycle close
handling. Idea 105 remains the downstream bridge follow-up and should own any
cleanup that needs HIR-to-LIR text/id bridge redesign or codegen/layout
identity changes.

## Suggested Next

Supervisor should review the completed Step 6 proof and route the active
runbook to plan-owner lifecycle close handling. The next implementation
initiative should be idea 105 bridge inventory/cleanup, not additional idea 104
HIR-only demotion work.

## Watchouts

- Do not retire `decl_lookup_*` records, owner parity mismatches, or Lowerer
  parity counters without a later review-backed proof that the corresponding
  rendered authority is no longer needed.
- Do not delete or weaken rendered fallbacks that serve `Node::name`,
  synthesized mangled names, `TypeSpec::tag`, trait/static evaluation,
  diagnostics/printers, ABI/link spelling, or HIR-to-LIR layout/codegen seams
  as part of idea 104 close handling.
- Any stronger removal now crosses into idea 105 bridge scope or removes
  proof-only observation before the audit says it is deletion-ready.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log`
passed 2976/2976.

Proof log: `test_after.log`.
