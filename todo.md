# Current Packet

Status: Active
Source Idea Path: ideas/open/788_lir_phi_incoming_successor_occurrence_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the selected PHI incoming occurrence source

## Just Finished

- Lifecycle switch: parent 734 Step 7.25 is paused after accepted safe partial
  Raw-BIR receiver `006d79aaf`; its stale unrelated current-step metadata and
  hook reminder were removed with this reset.

## Suggested Next

- Trace the selected `LirPhiIncoming` producer and verifier seams, then define
  the smallest native per-incoming successor-occurrence contract without
  changing Raw-BIR or inferring semantics from text/order.

## Watchouts

- A predecessor block ID alone is ambiguous for parallel conditional or switch
  successors; PHI input order is not authority.
- Keep 734's `006d79aaf` accepted partial receiver intact. Do not edit Raw-BIR,
  importer/container/verifier, lowering, or later LIR families.

## Proof

- Before implementation, supervisor prepares matching `^backend_` baseline.
- Packet acceptance requires fresh build, focused selected-family proof,
  matching `^backend_` 5/5 before-and-after guard with
  `--allow-non-decreasing-passed`, and fresh full `ctest` 3037/3037.
