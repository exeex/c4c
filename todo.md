# Current Packet

Status: Active
Source Idea Path: ideas/open/737_bir_phase_c_preparation_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Converge C4 call preparation

## Just Finished

- Completed plan Step 3 by converging
  `src/backend/bir/preparation/abi/README.md` as the sole C3 immutable ABI
  requirement-plan owner. Added exact `under-review`/`absent`/`product`/`C3`
  metadata, exact verifier-bound Canonical/C1 plus C2 and exact-current
  publication/value-flow upstream, exact C4 downstream, owner path and
  reconciliation point `069bd3093`.
- Reordered the contract to substantive core-first `Purpose`, `Owns`,
  `Does Not Own`, `Inputs`, `Outputs` and `Adjacent-Stage Contract` sections.
  Detailed identity, classification, transaction, failure, target and
  implementation clauses follow the core boundary.
- Added a ten-row input matrix for the exact `VerifiedPreparationInput`, B8
  Canonical stamp, complete C1 `TargetProfile`/fingerprint, matching C2
  `VerifiedTargetLayout`, publication/value-flow descriptor and handle bundle,
  ABI registry/schema/options and deterministic execution control.
- Bound each defined function to a fresh exact-current
  `PublicationValueFlow` handle after B8. Its function/revision, CFG and
  dominance dependencies must match exactly; its domain remains
  `CanonicalSemantic`, target `None`, preparation empty. Declaration-only and
  empty-module forms are explicit, and old B3/P04 handles fail stale even when
  their semantic facts compare equal.
- Added a ten-row output matrix. Exactly one immutable module-wide `AbiPlan`
  owns embedded function summaries, parameter/result, byval, hidden sret,
  aggregate/HFA/HVA, variadic-boundary and abstract preservation requirements,
  plus structured failure. Embedded tables are not separate capabilities.
- Closed the eight-axis `AbiPlanKey`: exact Canonical identity, C1 target, C2
  layout, ordered exact analysis identities, ABI rule registry, plan
  schema/options, stable requirement IDs and a complete verified-output
  fingerprint. Empty/zero axes remain present and compatible-looking products
  are rejected.
- Exhaustively classified 23 semantic boundary forms covering empty and
  declaration cases, zero/scalar/float/vector parameters and results, direct
  and split aggregates, byval, hidden sret, HFA/HVA, fixed and extra variadic
  domains, preservation, unsupported conventions, unknown semantic/analysis
  facts and legal/illegal memory fallbacks.
- Preserved the C3/C4 authority seam. C3 may publish register-eligible,
  stack-required, class/group, width/alignment/split and preservation
  requirements, but selects no concrete register, slot, stack offset, move,
  home, frame state or instruction and performs no call transport or lowering.
- Defined one private classify/verify transaction with stable ordering, total
  boundary coverage, all-or-nothing split/homogeneous closure, fallback
  legality, complete-key recheck and atomic publication. Failure publishes no
  plan, prefix, requirement, fingerprint, cache or C4 fact; invalidation is
  transitive through C4-C9 and old handles never retarget.
- Proved the exact C4 handoff against the read-only call-preparation contract:
  C4 already consumes the same verifier-bound input, matching target layout
  and complete `AbiPlan` fingerprint. No shared-document edit is needed.
- Reconciled implementation truth to `absent`. The C3 ABI directory contains
  only its README and has no build edge or checked-in plan/key/requirements,
  registry, analysis request route, verifier, transaction, cache, C4 handoff
  or focused runtime proof. Legacy ABI helpers remain migration evidence only.
- Re-audited exact scope, metadata/order, matrices, classification families,
  exact analysis key and timing, failure/invalidation, C4 acceptance, links and
  implementation truth. Only the C3 README and canonical `todo.md` changed; no
  code, tests, build, logs, plan, source idea or shared document changed.

## Suggested Next

- Execute plan Step 4 against `src/backend/bir/preparation/calls/README.md`:
  converge every call form against exact Canonical/C1/C2/C3 and earliest
  call-graph facts, publish immutable typed call/preservation/clobber/return
  requirements, and preserve D2 as the sole ABI-aware call-lowering authority.

## Watchouts

- C4 is the earliest call/helper consumer of call-graph facts. Request the
  exact-current result there; do not move it backward into C3 ABI signature
  classification or retag target-independent analysis as target-aware.
- Consume the complete same-key `AbiPlan`; do not reconstruct classifications,
  accept compatible-looking target/layout/analysis keys, or silently substitute
  a newly computed analysis result for an old key.
- Keep call preparation requirements-only. Concrete registers, moves, outgoing
  stack offsets, call lowering, target opcodes, homes and frame state remain
  later authorities.
- Cover direct, indirect, tail, variadic, helper, multi-result, byval and sret
  call forms with explicit optional/error forms and whole-product failure.
- C1-C3 remain documentation contracts without callable success paths; do not
  infer implementation from legacy call/return classification or target
  register helpers.

## Proof

- Documentation-only packet. The delegated scope forbids code, test, build and
  log edits; no canonical regression log was created or modified.
- Exact two-path scope, metadata/order, matrix/classification inventory,
  analysis key/timing, atomic failure, C4 handoff, links and
  implementation-truth proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

path = Path('src/backend/bir/preparation/abi/README.md')
text = path.read_text()
metadata = '\n'.join(text.splitlines()[:11])

for item in (
    'Contract-Status: under-review',
    'Implementation-Status: absent',
    'Kind: product',
    'Phase-ID: C3',
    'Upstream: exact verifier-bound Canonical/C1 input plus matching C2 `VerifiedTargetLayout` and exact-current `PublicationValueFlow`',
    'Downstream: one immutable exact-key `AbiPlan` consumed first by C4 call preparation',
    'Owner-Path: `src/backend/bir/preparation/abi/README.md`',
    'Last-Reconciled-Commit: `069bd3093`',
):
    assert item in metadata, item

core = ('## Purpose', '## Owns', '## Does Not Own', '## Inputs',
        '## Outputs', '## Adjacent-Stage Contract')
positions = [text.index(heading) for heading in core]
assert positions == sorted(positions)
assert positions[-1] < text.index('## Stable Identity and Exact Product Key')

def rows(heading, next_heading):
    body = text.split(heading, 1)[1].split(next_heading, 1)[0]
    return [line for line in body.splitlines() if line.startswith('|')][2:]

assert len(rows('### Exact C3 input matrix', '## Outputs')) == 10
assert len(rows('### Exact C3 output matrix', '## Adjacent-Stage Contract')) == 10
assert len(rows('## Stable Identity and Exact Product Key',
                '## Exhaustive ABI Classification Matrix')) == 8
assert len(rows('## Exhaustive ABI Classification Matrix',
                '## Ordered Behavior')) == 23

for item in (
    'VerifiedPreparationInput', 'VerifiedTargetLayout',
    'PublicationValueFlow', 'CanonicalSemantic', 'target `None`',
    'preparation empty', 'parameter requirements', 'result requirements',
    'by-value requirements', 'hidden-result requirements',
    'aggregate/HFA/HVA requirements', 'variadic boundary requirements',
    'by-value parameter/object', 'hidden sret parameter',
    'HFA/HVA parameter or result',
    'variadic ellipsis / extra-argument domain',
    'class/group absent with legal memory fallback',
    'class/group absent without legal fallback',
    'Failure publishes no plan', 'Old handles never retarget',
    'Implementation is absent', 'directory contains only this README',
):
    assert item in text, item

for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
    assert (path.parent / link).resolve().exists(), link

publication = Path('src/backend/bir/analysis/publication/README.md').read_text()
for item in ('PublicationValueFlow', 'schema 1', 'CanonicalSemantic',
             'target key `None`'):
    assert item in publication, item
calls = Path('src/backend/bir/preparation/calls/README.md').read_text()
for item in ('VerifiedPreparationInput', 'VerifiedTargetLayout', '`AbiPlan`'):
    assert item in calls, item

assert [entry.name for entry in path.parent.iterdir()] == ['README.md']
implementation = subprocess.run(
    ['rg', '-n', '--glob', '*.{cpp,hpp,h,cc,cxx}',
     'AbiPlan|AbiRequirementId|AbiPlanFingerprint', 'src/backend/bir'],
    text=True, capture_output=True)
assert implementation.returncode == 1, implementation.stdout

changed = subprocess.check_output(
    ['git', 'status', '--short', '--untracked-files=all'],
    text=True).splitlines()
changed_paths = {line[3:] for line in changed}
assert changed_paths == {
    'src/backend/bir/preparation/abi/README.md', 'todo.md'
}, changed_paths
assert subprocess.run(['git', 'diff', '--check']).returncode == 0
todo = Path('todo.md').read_text()
assert 'Current Step ID: 4' in todo
assert 'Current Step Title: Converge C4 call preparation' in todo

print('PASS step=3 metadata=8 core=6 matrices=10/10 key=8 '
      'classification=23 analysis=exact-current-after-B8-target-none '
      'failure=atomic c4=exact-same-key implementation=absent '
      'scope=abi+todo next=4')
PY
```

- Result:

```text
PASS step=3 metadata=8 core=6 matrices=10/10 key=8 classification=23 analysis=exact-current-after-B8-target-none failure=atomic c4=exact-same-key implementation=absent scope=abi+todo next=4
```
