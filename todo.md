# Current Packet

Status: Active
Source Idea Path: ideas/open/786_lir_phi_special_token_semantic_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish and verify PHI SpecialToken authority

## Just Finished

- Plan Step 1 complete: PHI inputs now carry native `LirSpecialToken`
  authority for the existing `null`, `undef`, `poison`, `zeroinitializer`,
  `true`, and `false` vocabulary. Relevant ternary/logical producers publish
  it, and PHI verification rejects absent, kind-mismatched, invalid, and
  misleading-display authority.

## Suggested Next

- Supervisor: assess Step 1 acceptance and record the precise 734 handoff;
  no Raw-BIR receiver work belongs in this packet.

## Watchouts

- Do not change Raw-BIR/importer/receiver code or recover semantics from token
  spelling, printer output, LLVM text, labels, instruction order, or testcase
  names.
- Keep the work limited to the existing classified SpecialToken vocabulary and
  relevant PHI producers; all other operand families remain out of scope.
- The verifier uses native token identity for semantics and checks spelling
  only as its compatibility mirror; it does not derive token identity from text.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`.
- Focused coverage includes valid PHI authority for every classified token and
  malformed absent, kind-mismatched, invalid, and misleading-display cases.
- Proof log: `test_after.log`.
