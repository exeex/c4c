# Current Packet

Status: Active
Source Idea Path: ideas/open/737_bir_phase_c_preparation_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Converge C6 address preparation

## Just Finished

- Completed plan Step 5 by converging
  `src/backend/bir/preparation/variadic/README.md` as the sole C5 immutable
  variadic requirement-plan owner. Added exact
  `under-review`/`absent`/`product`/`C5` metadata, exact verifier-bound
  Canonical/C1 plus C2/C3/C4 upstream, exact C6 and later D2 downstream, owner
  path and reconciliation point `1468f5a62`.
- Reordered the contract to substantive core-first `Purpose`, `Owns`,
  `Does Not Own`, `Inputs`, `Outputs` and `Adjacent-Stage Contract` sections.
  Detailed identity, form, transaction, failure, authority and implementation
  clauses follow the core boundary.
- Added a ten-row input matrix for the exact `VerifiedPreparationInput`, B8
  Canonical stamp, complete C1 profile/fingerprint, matching C2
  `VerifiedTargetLayout`, exact C3 `AbiPlan`, exact C4 `CallPlan`, ABI-selected
  variadic registry, schema/options and deterministic execution control.
- Added a fourteen-row output matrix. Exactly one immutable module-wide
  `VariadicPlan` owns function entry, fixed/named and extra/unnamed boundaries,
  default promotions, abstract register-save/overflow domains, `va_list` state,
  `va_start`/`va_copy`/`va_arg`/`va_end`, traversal/lifetime and structured
  failure requirements. Embedded tables are not independent capabilities.
- Closed nine exact `VariadicPlanKey` axes: Canonical, C1 target, C2 layout, C3
  ABI, C4 call, variadic registry, schema/options, stable requirement IDs and
  complete verified-output fingerprint. Empty/zero axes remain present and
  compatible-looking or reconstructed predecessor products fail.
- Exhaustively classified 31 forms covering empty/nonvariadic cases, variadic
  declarations and definitions with zero or fixed named parameters, calls with
  zero or multiple extras, identity/integer/floating promotions, admitted and
  rejected aggregate/vector/HFA/HVA forms, absent/present/exhausted save
  domains, every `va_*` operation, alignment, state merges and invalid lifetime
  or traversal paths.
- Bound every extra argument to exact C3 classification/promotion and C4 call-
  site rows. C5 proves already-present default promotions but inserts no cast,
  rewrites no operand and never recreates the fixed/extra boundary.
- Defined abstract register-save-area and overflow-domain semantic
  requirements from exact C2 eligible classes/capacities and ABI rules. These
  contain stable regions, alignment, lifetime, access, progression and fallback
  requirements but no chosen register/slot, concrete save-area object, address,
  byte/frame offset or placement.
- Defined typed `va_list` state and deterministic initialization/copy/
  traversal/end lifetime requirements, including distinct copied-state
  identities, exhaustion fallback, aggregate/HFA group closure and branch-merge
  proof. C5 mutates no list object and emits no load/store or instruction.
- Defined one private inventory/join/derive/traverse/verify transaction with
  total definition/call/operation coverage, stable identity, exact predecessor
  agreement, promotions, capacity/fallback, state/lifetime closure and complete-
  key recheck. Failure publishes no plan, prefix, requirement, fingerprint,
  cache, C6 or D2 fact and mutates no input.
- Proved adjacent seams read-only: C6 already accepts the exact ordered C3/C4/
  C5 fingerprints and cannot place or repair variadic state; D2 later owns ABI
  transport and semantic-operation pseudos; E4 alone places frame objects and
  actions. No shared-document repair is needed.
- Reconciled implementation truth to `absent`. The C5 directory contains only
  its README and has no build edge or checked-in plan/key/requirements,
  registry, planner/verifier transaction, state graph, cache, C6/D2 consumer
  path or focused runtime proof. Existing LIR import and legacy variadic/HFA
  helpers remain migration evidence only.
- Re-audited exact scope, metadata/order, matrices, form families, keys,
  atomic failure/invalidation, C6/D2 boundaries, links and implementation
  truth. Only the C5 README and canonical `todo.md` changed; no code, tests,
  build, logs, plan, source idea or shared document changed.

## Suggested Next

- Execute plan Step 6 against `src/backend/bir/preparation/address/README.md`:
  converge all address/global/local/relocation forms against exact predecessor
  and earliest provenance keys, publish immutable semantic materialization
  requirements, and preserve D4/E4/F1 legalization/placement/application
  authority.

## Watchouts

- C6 consumes the complete same-key C3/C4/C5 products. It must not reconstruct
  classifications, call boundaries or variadic traversal, accept compatible-
  looking products, or silently substitute predecessor identity.
- Request exact-current provenance only at its earliest address consumer and
  keep the analysis target/preparation-independent; stale handles fail closed.
- Distinguish semantic base/index/scale/displacement/relocation and dynamic-
  object requirements from a chosen register, instruction sequence, frame
  base/offset, helper, scratch, home or machine opcode.
- C7 is the immediate consumer; D1/D4 later form/legalize pseudos, E4 places
  frame objects/actions and F1 only maps directly realizable nodes one-to-one.
- C1-C5 remain documentation-only without callable success paths; do not infer
  implementation from legacy addressing, decoded-storage or target helpers.

## Proof

- Documentation-only packet. The delegated scope forbids code, test, build and
  log edits; no canonical regression log was created or modified.
- Exact two-path scope, metadata/order, matrices, variadic-form inventory,
  complete key, atomic failure, C6/D2 boundary, links and implementation-truth
  proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

path = Path('src/backend/bir/preparation/variadic/README.md')
text = path.read_text()
metadata = '\n'.join(text.splitlines()[:11])

for item in (
    'Contract-Status: under-review',
    'Implementation-Status: absent',
    'Kind: product',
    'Phase-ID: C5',
    'Upstream: exact verifier-bound Canonical/C1 input, matching C2 `VerifiedTargetLayout`, exact C3 `AbiPlan`, and exact C4 `CallPlan`',
    'Downstream: one immutable exact-key `VariadicPlan` consumed first by C6 address preparation and later by D2 call lowering',
    'Owner-Path: `src/backend/bir/preparation/variadic/README.md`',
    'Last-Reconciled-Commit: `1468f5a62`',
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

assert len(rows('### Exact C5 input matrix', '## Outputs')) == 10
assert len(rows('### Exact C5 output matrix',
                '## Adjacent-Stage Contract')) == 14
assert len(rows('## Stable Identity and Exact Product Key',
                '## Exhaustive Variadic Form Matrix')) == 9
assert len(rows('## Exhaustive Variadic Form Matrix',
                '## Ordered Behavior')) == 31

for item in (
    'VerifiedPreparationInput', 'VerifiedTargetLayout', '`AbiPlan`',
    '`CallPlan`', 'VariadicPlanSchemaFingerprint',
    'VariadicPlanOptionsFingerprint', 'nonvariadic function',
    'variadic definition with fixed parameters',
    'variadic call with extras', 'default-promotion requirements',
    'narrow integer/bool/enum extra',
    'floating extra requiring default promotion',
    'register-save-area semantic requirements',
    'class capacity exhausted with legal overflow', 'valid `va_start`',
    'valid `va_copy`', 'scalar integer/pointer `va_arg`',
    'aggregate/HFA/HVA `va_arg`', 'valid `va_end`',
    'Failure publishes no plan', 'Old handles never retarget',
    'Implementation is absent', 'directory contains only this README',
):
    assert item in text, item

for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
    assert (path.parent / link).resolve().exists(), link

abi = Path('src/backend/bir/preparation/abi/README.md').read_text()
for item in ('`AbiPlanKey`', '`AbiPlanFingerprint`',
             'variadic boundary requirements'):
    assert item in abi, item
calls = Path('src/backend/bir/preparation/calls/README.md').read_text()
for item in ('`CallPlanKey`', '`CallPlanFingerprint`',
             'variadic-call boundary'):
    assert item in calls, item
address = Path('src/backend/bir/preparation/address/README.md').read_text()
address_flat = ' '.join(address.split())
for item in ('variadic products',
             'exact ordered ABI/call/variadic product fingerprints'):
    assert item in address_flat, item
d2 = Path('src/backend/bir/passes/call_lowering/README.md').read_text()
for item in ('D2 is the sole owner', 'variadic transport'):
    assert item in d2, item
verifier = Path('src/backend/bir/verify/README.md').read_text()
for item in ('`va_start`, scalar/aggregate `va_arg`, `va_copy`,',
             'Register-save areas, GP/FP offsets,'):
    assert item in verifier, item

assert [entry.name for entry in path.parent.iterdir()] == ['README.md']
implementation = subprocess.run(
    ['rg', '-n', '--glob', '*.{cpp,hpp,h,cc,cxx}',
     'VariadicPlan|VariadicRequirementId|VariadicPlanFingerprint',
     'src/backend/bir'], text=True, capture_output=True)
assert implementation.returncode == 1, implementation.stdout

changed = subprocess.check_output(
    ['git', 'status', '--short', '--untracked-files=all'],
    text=True).splitlines()
changed_paths = {line[3:] for line in changed}
assert changed_paths == {
    'src/backend/bir/preparation/variadic/README.md', 'todo.md'
}, changed_paths
assert subprocess.run(['git', 'diff', '--check']).returncode == 0
todo = Path('todo.md').read_text()
assert 'Current Step ID: 6' in todo
assert 'Current Step Title: Converge C6 address preparation' in todo

print('PASS step=5 metadata=8 core=6 matrices=10/14 key=9 forms=31 '
      'promotion+entry+save+va_state=closed failure=atomic '
      'c6=exact d2=transport-owner implementation=absent '
      'scope=variadic+todo next=6')
PY
```

- Result:

```text
PASS step=5 metadata=8 core=6 matrices=10/14 key=9 forms=31 promotion+entry+save+va_state=closed failure=atomic c6=exact d2=transport-owner implementation=absent scope=variadic+todo next=6
```
