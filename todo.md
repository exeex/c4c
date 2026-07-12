# Current Packet

Status: Active
Source Idea Path: ideas/open/723_pre_regalloc_value_constraint_carrier_research.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define deterministic proof and follow-up

## Just Finished

- Plan Step 3 defined a deterministic semantic positive and proof boundary in `docs/pre_regalloc_value_constraints/03_proof_and_followup_boundary.md`: two independently meaningful explicit-register inline-assembly operand constraints force distinct predecessor/phi identities and make the edge move a downstream consequence.
- The document includes missing, stale, ambiguous, mismatch, unsupported, class/width, home, interference, preference, forbidden-pool, publication, and freshness negatives; deterministic acceptance rules; forbidden shortcuts; and a narrow prepared inline-assembly constraint implementation outline with a stop condition.

## Suggested Next

- Execute Plan Step 4 by writing the research index, checking exact file count and cross-document consistency, and reviewing the set against idea 723 reject signals.

## Watchouts

- Current inline-assembly code validates physical identities after homes exist; the follow-up is valid only if supported explicit-register meaning can be preserved before regalloc. If that requires broad syntax redesign, stop rather than substituting a fixture map.

## Proof

- No build was required for this documentation-only packet.
- Ran repository citation checks for inline-assembly metadata, identity parsers, carrier validation, prepared constraints, candidate selection, and the direct-edge typed query; all cited paths and symbols resolved. `git diff --check` passed.
