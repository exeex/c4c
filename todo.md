# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair B5-B8 identifiers and Canonical publication

## Just Finished

- Plan Step 6 repaired obsolete identifiers across B5 / P05 memory, B6 / P06
  aggregate, B7 / P07 intrinsics, and the B8 Canonical publication gate.
- Bound B8 to only the exact frozen B7 / P07 occurrence; diagnostic-only
  verification, copied stamps, equal semantic hashes, source gaps, and partial
  implementation adapters cannot mint or weaken a publication capability.
- Closed reachability semantics: Raw admits and fully checks unreachable
  components, while Canonical requires B3 / P03 entry reachability for every
  retained block.
- Plan Step 6 reconciled the verifier with A2's closed core choices for named
  records, equal-target Raw branches and P03 folding, local unwind rejection,
  `SemanticDataLayout`, debug/origin IDs, and canonical eager core def-use.
- Removed the corresponding stale review questions; only the C9 API and
  Canonical-versus-`PreparedInput` questions remain open and assigned to Step 7.

## Suggested Next

- Execute Plan Step 7, "Close target-preparation and verifier-profile
  boundaries."

## Watchouts

- Keep B8 as the only Canonical publication gate and preserve Raw's
  unreachable-component allowance plus Canonical's post-B3 / P03 reachability
  requirement.
- Step 7 owns the two remaining verifier questions: the exact C9 binding API and
  the pass promise distinguishing Canonical from `PreparedInput`.
- Preserve A2's typed core authorities; Step 7 must not move semantic layout,
  debug/origin identity, local unwind topology, or value def-use into a later
  product or analysis.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && ! rg -n 'S0[0-9]|S1[0-9]|S2[0-9]|G01' src/backend/bir/passes/memory/README.md src/backend/bir/passes/aggregate/README.md src/backend/bir/passes/intrinsics/README.md src/backend/bir/verify/README.md && rg -n 'B4|B5|B6|B7|B8|Raw|Canonical|reachab|P07|transaction|publish|source gap|implementation' src/backend/bir/passes/memory/README.md src/backend/bir/passes/aggregate/README.md src/backend/bir/passes/intrinsics/README.md src/backend/bir/verify/README.md`.
- Passed: `git diff --check && ! rg -n 'Are exception/invoke edges|required in the first complete Raw|Will debug provenance|Should cached def-use|Which single service owns aggregate object layout|Is unreachable code accepted' src/backend/bir/verify/README.md && rg -n 'TypeNameId|equal true/false|MayUnwind|SemanticDataLayout|Debug|OriginId|core.*def-use|Step 7|C9|PreparedInput' src/backend/bir/verify/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
