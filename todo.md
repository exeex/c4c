# Current Packet

Status: Active
Source Idea Path: ideas/open/737_bir_phase_c_preparation_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Converge C5 variadic preparation

## Just Finished

- Completed plan Step 4 by converging
  `src/backend/bir/preparation/calls/README.md` as the sole C4 immutable
  per-call requirement-plan owner. Added exact
  `under-review`/`absent`/`product`/`C4` metadata, exact verifier-bound
  Canonical/C1 plus C2/C3 and exact-current call-graph upstream, exact C5 and
  later D2 downstream, owner path and reconciliation point `954166281`.
- Reordered the contract to substantive core-first `Purpose`, `Owns`,
  `Does Not Own`, `Inputs`, `Outputs` and `Adjacent-Stage Contract` sections.
  Detailed identity, call-form, transaction, failure, authority and
  implementation clauses follow the core boundary.
- Added an eleven-row input matrix for the exact `VerifiedPreparationInput`,
  B8 Canonical stamp, complete C1 profile/fingerprint, matching C2
  `VerifiedTargetLayout`, exact C3 `AbiPlan`, call-graph descriptor and handle,
  call-rule registry, schema/options and deterministic execution control.
- Bound C4 to a fresh exact-current B8 `CallGraph`: exact epoch/module revision
  and ordered complete function/body digest, empty dependencies/options,
  `CanonicalSemantic`, target-layout `None` and preparation empty. An old
  B6/P07 handle is stale even when checked preservation could publish an equal
  new-key result; no handle implicitly recomputes or retargets.
- Added a fourteen-row output matrix. Exactly one immutable module-wide
  `CallPlan` owns call summaries, typed inputs/outputs, simultaneous-transfer,
  outgoing-object, hidden-carrier, preservation, clobber, return, tail,
  variadic-boundary and deferred-helper requirements plus structured failure.
  Embedded tables are not independent capabilities.
- Closed nine exact `CallPlanKey` axes: Canonical, C1 target, C2 layout, C3 ABI,
  exact-current analysis, call-rule registry, schema/options, stable
  requirement IDs and complete verified-output fingerprint. Empty/zero axes
  remain present; compatible-looking or separately reconstructed products fail.
- Exhaustively classified 27 call/operation forms: empty, internal/external
  direct, proven finite indirect, legal/illegal unknown indirect, zero/single/
  multi-result, byval, sret, ordinary/optional/required tail, nonvariadic and
  variadic fixed/extra forms, call-like/non-call intrinsics, helper-eligible
  semantic operations, inline assembly, malformed input, predecessor
  disagreement and unsupported forms.
- Covered typed input/output, preservation/clobber and return requirements
  explicitly while preserving authority. C4 publishes requirement groups and
  abstract object needs, but no move sequence, ABI slot, concrete register,
  stack offset, store/call/result move, save/restore, home, frame state, target
  opcode or encoding.
- Kept helper selection out of C4. Helper-eligible Canonical operations receive
  typed `DeferredHelperCallRequirement` rows with explicit call-graph non-call
  status; C8 alone selects an interface, D1 alone forms `GenericCall`, and D2
  alone performs shared ABI-aware transport/lowering.
- Defined one private inventory/join/classify/verify transaction with total
  call/helper coverage, stable identity, exact C3 reference, signature/effect
  agreement, finite-set/fallback/tail legality, all-or-nothing groups and
  complete-key recheck. Failure publishes no plan, prefix, requirement,
  fingerprint, cache, C5/helper/D2 fact and mutates no input.
- Proved adjacent seams read-only: C5 accepts only the complete same-key
  `CallPlan`; C8 alone selects helpers; D2 consumes exact C3/C4 fingerprints
  through the cumulative bundle and alone creates call transport nodes. No
  shared-document repair is needed.
- Reconciled implementation truth to `absent`. The C4 directory contains only
  its README and has no build edge or checked-in plan/key/requirements,
  registry, exact-current analysis route, verifier, transaction, cache,
  C5/C8/D2 consumer path or focused runtime proof. Legacy call-planning and
  target-private helpers remain migration evidence only.
- Re-audited exact scope, metadata/order, matrices, call families, exact
  analysis key/timing, atomic failure/invalidation, C5/C8/D2 boundaries, links
  and implementation truth. Only the C4 README and canonical `todo.md`
  changed; no code, tests, build, logs, plan, source idea or shared document
  changed.

## Suggested Next

- Execute plan Step 5 against `src/backend/bir/preparation/variadic/README.md`:
  converge exact fixed/variadic entry and call forms, promotions, abstract save-
  area/traversal requirements, predecessor keys, verifier/failure/invalidation
  and the C6 handoff without frame placement or ABI transport.

## Watchouts

- C5 consumes the complete same-key C3/C4 plans. It must not reconstruct
  classifications or per-call boundaries, accept compatible-looking products,
  or silently substitute a new analysis result for predecessor identity.
- Preserve the fixed/extra boundary and C4 promotion obligations exactly.
  Variadic entry, zero extras, register/class exhaustion, aggregate/floating
  variants and unsupported traversal need explicit optional/error forms.
- C5 may publish abstract save-area objects and traversal requirements but no
  concrete ABI location, register save sequence, stack/frame offset, `va_list`
  mutation instruction, call transport, target opcode, home or frame action.
- C6 is the immediate consumer; D2 later transports from the verified
  cumulative products. Neither may repair or weaken a C5 plan.
- C1-C4 and the call-graph contract remain documentation-only without callable
  success paths; do not infer implementation from legacy variadic/call helpers.

## Proof

- Documentation-only packet. The delegated scope forbids code, test, build and
  log edits; no canonical regression log was created or modified.
- Exact two-path scope, metadata/order, matrices, call-family inventory,
  exact-current analysis key/timing, atomic failure, C5/C8/D2 boundaries,
  links and implementation-truth proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

path = Path('src/backend/bir/preparation/calls/README.md')
text = path.read_text()
metadata = '\n'.join(text.splitlines()[:11])

for item in (
    'Contract-Status: under-review',
    'Implementation-Status: absent',
    'Kind: product',
    'Phase-ID: C4',
    'Upstream: exact verifier-bound Canonical/C1 input, matching C2 `VerifiedTargetLayout`, exact C3 `AbiPlan`, and exact-current `CallGraph`',
    'Downstream: one immutable exact-key `CallPlan` consumed first by C5 variadic preparation and later by D2 call lowering',
    'Owner-Path: `src/backend/bir/preparation/calls/README.md`',
    'Last-Reconciled-Commit: `954166281`',
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

assert len(rows('### Exact C4 input matrix', '## Outputs')) == 11
assert len(rows('### Exact C4 output matrix',
                '## Adjacent-Stage Contract')) == 14
assert len(rows('## Stable Identity and Exact Product Key',
                '## Exhaustive Call-Form Matrix')) == 9
assert len(rows('## Exhaustive Call-Form Matrix',
                '## Ordered Behavior')) == 27

for item in (
    'VerifiedPreparationInput', 'VerifiedTargetLayout', '`AbiPlan`',
    'AnalysisId::CallGraph', 'CanonicalSemantic',
    'target-layout key `None`', 'preparation digest',
    'typed input requirements', 'typed output requirements',
    'preservation requirements', 'clobber requirements',
    'return requirements', 'direct internal call',
    'proven finite-set indirect call',
    'unknown-indirect call with registered ABI fallback',
    'variadic call with extras', 'call-like intrinsic retained by P07',
    'helper-eligible semantic operation', 'DeferredHelperCallRequirement',
    'Failure publishes no plan', 'Old handles never retarget',
    'Implementation is absent', 'directory contains only this README',
):
    assert item in text, item

for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
    assert (path.parent / link).resolve().exists(), link

call_graph = Path('src/backend/bir/analysis/call_graph/README.md').read_text()
for item in ('AnalysisId::CallGraph', 'schema 1', 'Module',
             'CanonicalSemantic', 'target key `None`',
             'empty preparation digest', 'old handle never rebinds'):
    assert item in call_graph, item
variadic = Path('src/backend/bir/preparation/variadic/README.md').read_text()
for item in ('VerifiedPreparationInput', 'VerifiedTargetLayout',
             '`AbiPlan`', '`CallPlan`'):
    assert item in variadic, item
d2 = Path('src/backend/bir/passes/call_lowering/README.md').read_text()
for item in ('D2 is the sole owner', 'exact `AbiPlan` and `CallPlan`',
             '`AbiPreserve`', '`AbiArgMove`', '`AbiArgStore`', '`AbiCall`',
             '`AbiResultMove`', '`AbiRestore`'):
    assert item in d2, item
helpers = Path('src/backend/bir/preparation/runtime_helpers/README.md').read_text()
assert 'C8 is the sole eligibility owner' in helpers

assert [entry.name for entry in path.parent.iterdir()] == ['README.md']
implementation = subprocess.run(
    ['rg', '-n', '--glob', '*.{cpp,hpp,h,cc,cxx}',
     'CallPlan|CallRequirementId|CallPlanFingerprint', 'src/backend/bir'],
    text=True, capture_output=True)
assert implementation.returncode == 1, implementation.stdout

changed = subprocess.check_output(
    ['git', 'status', '--short', '--untracked-files=all'],
    text=True).splitlines()
changed_paths = {line[3:] for line in changed}
assert changed_paths == {
    'src/backend/bir/preparation/calls/README.md', 'todo.md'
}, changed_paths
assert subprocess.run(['git', 'diff', '--check']).returncode == 0
todo = Path('todo.md').read_text()
assert 'Current Step ID: 5' in todo
assert 'Current Step Title: Converge C5 variadic preparation' in todo

print('PASS step=4 metadata=8 core=6 matrices=11/14 key=9 forms=27 '
      'analysis=exact-current-after-B8-target-none failure=atomic '
      'c5=exact c8=helper-owner d2=transport-owner implementation=absent '
      'scope=calls+todo next=5')
PY
```

- Result:

```text
PASS step=4 metadata=8 core=6 matrices=11/14 key=9 forms=27 analysis=exact-current-after-B8-target-none failure=atomic c5=exact c8=helper-owner d2=transport-owner implementation=absent scope=calls+todo next=5
```
