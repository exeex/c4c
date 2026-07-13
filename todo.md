# Current Packet

Status: Active
Source Idea Path: ideas/open/737_bir_phase_c_preparation_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Converge C3 ABI preparation

## Just Finished

- Completed plan Step 2 by converging
  `src/backend/bir/target_layout/README.md` as the sole C2 finite target-layout
  product owner. Added exact `under-review`/`absent`/`product`/`C2` metadata,
  accepted-C1 upstream, exact C3 downstream, owner path and reconciliation
  point `eab55fde0`.
- Reordered the contract to substantive core-first `Purpose`, `Owns`,
  `Does Not Own`, `Inputs`, `Outputs` and `Adjacent-Stage Contract` sections,
  with every detailed stable-ID, table, transaction, failure, target and
  implementation rule following the core boundary.
- Added a nine-row input matrix for the exact immutable C1 validated
  `TargetProfile`, complete `TargetFingerprint`, verifier-issued
  `VerifiedPreparationInput`, exact Canonical `PipelineStageStamp`, profile/
  layout/mapping schemas, canonical empty semantic options and deterministic
  execution control. Architecture/triple similarity, rendered `data_layout`,
  Raw/Canonical target facts and imported allocator/prepared state are rejected.
- Added an eleven-row output matrix. Exactly one immutable
  `VerifiedTargetLayout` owns embedded category, class, group, slot, alias,
  reservation/eligibility, capacity, ABI-eligibility and private mapping-domain
  tables plus structured failure. No table or fingerprint is an independent
  capability.
- Closed eight stable ID/key rows for category, class, group, slot, alias unit,
  mapping rule, full `TargetLayoutKey` and complete layout fingerprint. Every
  downstream handle retains exact Canonical, target, profile/layout/mapping
  schema and options identity; old handles never retarget.
- Exhaustively enumerated eight category/class rows, eleven profile/capability
  slot/capacity domains, eight alias/reservation/capacity rules, four ABI
  eligibility families and nine exact private concrete-mapping domains.
  Stable symbolic slots, reserved units, optional/absent classes, known-zero
  capacities, shared float/vector alias units,
  legal group bases and private concrete mappings are explicit.
- Preserved authority separation. C1 alone validates/fingerprints the target;
  the verifier alone binds it to Canonical; C2 alone derives/verifies finite
  layout; C3 alone classifies per-value ABI requirements. C2 selects no value
  slot, home, placement, constraint, spill, frame or machine operation and
  requests no semantic analysis.
- Defined one private derivation/verification transaction with deterministic
  stable-ID order, complete-reference/alias/reservation/capacity/group/ABI/
  mapping validation and atomic publication. Failure publishes no layout,
  prefix, capacity, verifier token, cache or C3 fact and mutates no input.
- Proved the C3 handoff against the read-only ABI contract: C3 already consumes
  the exact `VerifiedPreparationInput` and matching `VerifiedTargetLayout`, and
  keys `AbiPlan` to the complete Canonical stamp, target fingerprint and layout
  schema. No shared-document repair is needed.
- Reconciled implementation truth to `absent`. The C2 directory contains only
  this README and has no build edge, IDs/key, tables, verifier, transaction,
  cache, capability or C3 handoff. Legacy concrete register pools and rendered/
  allocator data are migration evidence only.
- Re-audited exact scope, metadata/order, matrices/table families, stable keys,
  links, failure/invalidation, C3 acceptance and implementation truth. Only the
  C2 README and canonical `todo.md` changed; no code, tests, build, logs, plan,
  source idea or shared document changed.

## Suggested Next

- Execute plan Step 3 against `src/backend/bir/preparation/abi/README.md`:
  converge exact Canonical/C1/C2/publication-value-flow inputs, immutable
  `AbiPlan` requirements-only matrices, verifier/failure/invalidation and the
  C4 handoff without selecting concrete locations or performing call transport.

## Watchouts

- C3 is the first real publication/value-flow analysis consumer. Request the
  exact-current result there; do not move it backward into C1 selection or C2
  target-layout derivation.
- `VerifiedTargetLayout` publishes eligibility and finite capacity, not a
  per-value choice. C3 may classify register-eligible versus stack-required
  and group requirements but cannot choose slots, stack offsets, moves, homes
  or target instructions.
- Shared float/vector views share alias units and capacity. C3 must not double
  count them or infer a missing class/group from architecture register counts.
- Keep every C3 product keyed to the exact C1 target fingerprint, verifier-bound
  Canonical stamp, C2 layout fingerprint and ABI schema/options. Compatible-
  looking or predecessor-keyed products fail closed.
- C1/C2 remain documentation contracts without callable success paths; do not
  infer implementation from legacy target-register-profile helpers.

## Proof

- Documentation-only packet. The delegated scope forbids code, test, build and
  log edits; no `test_after.log` was created or modified.
- Exact two-path scope, metadata/order, matrix/table inventory, stable-key,
  authority, exclusion, atomic-failure, C3-handoff and implementation-truth
  proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

path = Path('src/backend/bir/target_layout/README.md')
text = path.read_text()
flat = ' '.join(text.split())
metadata = '\n'.join(text.splitlines()[:11])

for item in (
    'Contract-Status: under-review',
    'Implementation-Status: absent',
    'Kind: product',
    'Phase-ID: C2',
    'Upstream: exact C1 validated `TargetProfile` and complete `TargetFingerprint` plus verifier-bound `VerifiedPreparationInput`',
    'Downstream: one immutable exact-key `VerifiedTargetLayout` consumed first by C3 ABI preparation',
    'Owner-Path: `src/backend/bir/target_layout/README.md`',
    'Last-Reconciled-Commit: `eab55fde0`',
):
    assert item in metadata, item

core = ('## Purpose', '## Owns', '## Does Not Own', '## Inputs',
        '## Outputs', '## Adjacent-Stage Contract')
positions = [text.index(heading) for heading in core]
assert positions == sorted(positions), positions
assert '- [ ]' not in text

def rows(start, end):
    section = text.split(start, 1)[1].split(end, 1)[0]
    return [line for line in section.splitlines() if line.startswith('|')][2:]

inventory = {
    'inputs': rows('### Exact C2 input matrix', '## Outputs'),
    'outputs': rows('### Exact C2 output matrix',
                    '## Adjacent-Stage Contract'),
    'ids': rows('## Stable ID and Product-Key Schema',
                '## Closed Category, Class, and Group Matrix'),
    'classes': rows('## Closed Category, Class, and Group Matrix',
                    '## Closed Per-Profile Slot and Capacity Matrix'),
    'profiles': rows('## Closed Per-Profile Slot and Capacity Matrix',
                     '## Alias, Reservation, and Capacity Rules'),
    'aliases': rows('## Alias, Reservation, and Capacity Rules',
                    '## Exact ABI Eligibility Matrix'),
    'abi': rows('## Exact ABI Eligibility Matrix',
                '## Private Concrete-Mapping Domain'),
    'mappings': rows('## Private Concrete-Mapping Domain',
                     '## Ordered Behavior'),
}
assert {name: len(table) for name, table in inventory.items()} == {
    'inputs': 9, 'outputs': 11, 'ids': 8, 'classes': 8,
    'profiles': 11, 'aliases': 8, 'abi': 4, 'mappings': 9,
}

for item in (
    'validated profile', 'target fingerprint', 'preparation input',
    'Canonical stamp', 'target-profile schema', 'layout schema',
    'layout options', 'mapping registry', 'execution control',
):
    assert any(item in row for row in inventory['inputs']), item
for item in (
    '`VerifiedTargetLayout` capability', 'category table', 'class table',
    'group table', 'abstract slot table', 'alias relation',
    'reserved/eligible sets', 'capacity table', 'ABI eligibility table',
    'private concrete-mapping domain', 'structured `TargetLayoutFailure`',
):
    assert any(item in row for row in inventory['outputs']), item

for stable_id in (
    '`PseudoCategoryId`', '`PseudoClassId`', '`PseudoGroupId`',
    '`PseudoSlotId`', '`AliasUnitId`', '`MappingRuleId`',
    '`TargetLayoutKey`', '`TargetLayoutFingerprint`',
):
    assert any(stable_id in row for row in inventory['ids']), stable_id
for profile in (
    'RV64 LP64 `Gpr`', 'RV64 LP64 soft-float `Fpr`',
    'RV64 LP64F `Fpr`', 'RV64 LP64D `Fpr`',
    'RV64 `V` capability `Vreg`', 'AArch64 AAPCS64 `Gpr`',
    'AArch64 AAPCS64 float/vector shared domain',
    'x86-64 SysV `Gpr`', 'x86-64 SysV float/vector shared domain',
    'i686 SysV `Gpr`', 'i686 SysV SSE2 float/vector shared domain',
):
    assert any(profile in row for row in inventory['profiles']), profile

# Exact keys, exclusions and authority separation.
for item in (
    'architecture/triple-only reconstruction', 'rendered `data_layout`',
    'allocator/prepared state', 'complete `TargetFingerprint`',
    'exact B8 `PipelineStageStamp`', 'normalized semantic `TargetLayoutOptionsFingerprint`',
    'One complete `TargetLayoutKey` names every table',
    'old handle never retargets', 'C2 never requests publication/value-flow',
    'accepts only that capability with the same target fingerprint',
    'C2 publishes eligibility only',
    'No eligibility row selects a slot for a particular value',
):
    assert item in flat, item
for forbidden in ('selected home', 'spill/frame fact', 'machine operation'):
    assert forbidden in flat, forbidden

# Atomic verifier/failure and invalidation.
for item in (
    'The C2 verifier is part of the private publication transaction',
    'Failure publishes no layout, table prefix, capacity, verifier token, cache entry or C3 fact',
    'Inputs remain unchanged',
    'invalidates the capability and every C3-C9 successor',
    'An old handle never retargets',
):
    assert item in flat, item

# C1/C2/C3 links and the read-only C3 acceptance contract are already exact.
for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
    assert (path.parent / link.split('#', 1)[0]).resolve().exists(), link
c1 = Path('src/target_profile/README.md').read_text()
assert 'Last-Reconciled-Commit: `bd0a13414`' in c1
assert 'C2 target layout' in c1
abi = Path('src/backend/bir/preparation/abi/README.md').read_text()
assert ('consumes the exact\n`VerifiedPreparationInput` borrow and matching '
        '`VerifiedTargetLayout`') in abi
for item in ('complete Canonical `PipelineStageStamp`',
             'exact\n`TargetFingerprint`',
             'target-layout schema fingerprint'):
    assert item in abi, item
root = Path('src/backend/bir/README.md').read_text()
stage_ids = [
    re.match(r'^\| `([A-F][0-9])`', line).group(1)
    for line in root.splitlines()
    if re.match(r'^\| `[A-F][0-9]`', line)
]
assert stage_ids.index('C2') + 1 == stage_ids.index('C3')

# Implementation/build truth is absent, not legacy coverage.
assert {entry.name for entry in path.parent.iterdir()} == {'README.md'}
code = '\n'.join(
    candidate.read_text()
    for candidate in Path('src/backend/bir').rglob('*')
    if candidate.is_file() and candidate.suffix in {'.hpp', '.cpp'}
)
for absent in ('VerifiedTargetLayout', 'TargetLayoutKey',
               'TargetLayoutFingerprint'):
    assert absent not in code, absent
for item in (
    'Implementation is absent', 'This directory contains only this README',
    'it has no build edge', 'No checked-in `VerifiedTargetLayout`',
    'migration evidence only', 'do not consume the exact C1/Canonical binding',
):
    assert item in flat, item

# Exact writable scope and Step-3 handoff.
changed = subprocess.check_output(
    ['git', 'status', '--short', '--untracked-files=all'],
    text=True).splitlines()
changed_paths = {line[3:] for line in changed}
assert changed_paths == {
    'src/backend/bir/target_layout/README.md', 'todo.md'
}, changed_paths
assert subprocess.run(['git', 'diff', '--check']).returncode == 0
todo = Path('todo.md').read_text()
assert 'Current Step ID: 3' in todo
assert 'Current Step Title: Converge C3 ABI preparation' in todo

print('PASS step=2 metadata=8 core=6 matrices=9/11 ids=8 classes=8 '
      'profiles=11 aliases=8 abi=4 mappings=9 key=canonical+target+schemas+options '
      'failure=atomic c3=exact-matching-layout implementation=absent '
      'links=3 scope=target-layout+todo next=3')
PY
```

- Result:

```text
PASS step=2 metadata=8 core=6 matrices=9/11 ids=8 classes=8 profiles=11 aliases=8 abi=4 mappings=9 key=canonical+target+schemas+options failure=atomic c3=exact-matching-layout implementation=absent links=3 scope=target-layout+todo next=3
```
