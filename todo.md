# Current Packet

Status: Active
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Converge call-graph dependency and B7 P07 intrinsics

## Just Finished

- Completed plan Step 6 by converging B6/P06 aggregate canonicalization over
  the exact immutable B5 `MemoryCanonical` module wave.
- Added uniform metadata/core-first structure plus nine-row input and seven-row
  output matrices binding exact epoch/module/function revisions, cumulative
  P01-P05 properties, semantic type/value/path identity, opaque/later forms and
  target/helper exclusion.
- Added 43 exhaustive aggregate value/copy/projection/vector/by-value rows: ten
  `Normalize`, 23 `Preserve` and ten `Reject`. Every row has one disposition,
  exact result/stable failure, B7 visibility and truthful absent implementation.
- Preserved stable type/value/field/index identities, layout-independent paths,
  source-semantic attributes and exact CFG/SSA/memory/phi semantics. Fixed and
  scalable vector semantics remain portable; physical lane realization,
  aggregate decomposition, helper selection and ABI placement are excluded.
- Defined one deterministic private whole-module transaction, complete
  rollback, cumulative Raw+P01+P02+P03+P04+P05+P06 postconditions,
  verifier-on-commit and mutation-derived transitive invalidation.
- Defined exact B7 acceptance of one immutable B6 `AggregatesCanonical` wave.
  No partial prefix, B5 alias, analysis report or similar reconstruction may
  substitute.
- Confirmed implementation remains absent: the owner directory contains only
  its contract and no checked-in C/C++ definition names P06. No shared-owner
  seam remains, so the packet advances to Step 7.

## Suggested Next

- Execute plan Step 7 in required order: converge exact call-graph facts,
  re-audit/recompute memory effects by exact key, then B7/P07 intrinsics and
  its B8 candidate handoff.

## Watchouts

- P07 must consume the exact B6 wave and matching call/effect facts while
  preserving all P01-P06 forms. It cannot select helpers or interpret target
  features, ABI locations or opaque inline-asm text/constraints.
- Call-graph and memory-effects handles must be exact-current after P06 type,
  call-boundary or use changes; old B5/B6-crossing handles remain stale unless
  checked new-key installation succeeds.
- P06 implementation is absent. Its 43-row matrix is design authority, not
  runtime coverage.

## Proof

- Documentation-only packet. The delegated scope forbids code, tests, builds
  and log edits; canonical regression logs were not touched.
- Exact scope/diff-check, metadata/order/matrices/form counts, transaction,
  invalidation, target exclusion, B7 handoff, links and implementation proof:

```bash
python3 - <<'PY'
from collections import Counter
from pathlib import Path
import re
import subprocess

aggregate_path = Path('src/backend/bir/passes/aggregate/README.md')
aggregate = aggregate_path.read_text()
b5 = Path('src/backend/bir/passes/memory/README.md').read_text()
b7 = Path('src/backend/bir/passes/intrinsics/README.md').read_text()
framework = Path('src/backend/bir/analysis/README.md').read_text()
pipeline = Path('src/backend/bir/pipeline/README.md').read_text()

head = '\n'.join(aggregate.splitlines()[:11])
for item in ('Contract-Status: under-review',
             'Implementation-Status: absent', 'Kind: pass',
             'Phase-ID: B6 / P06', 'Upstream:', 'Downstream:',
             'Owner-Path:', 'Last-Reconciled-Commit:'):
    assert item in head, (item, head)

def ordered(text, headings):
    positions = [text.index(heading) for heading in headings]
    assert positions == sorted(positions), (headings, positions)

core = ['## Purpose', '## Owns', '## Does Not Own', '## Inputs',
        '## Outputs', '## Adjacent-Stage Contract']
details = ['## Ordered Behavior', '## Invariants',
           '## Exhaustive P06 Aggregate-Form Disposition Matrix',
           '## Verification and Publication',
           '## Failure and Diagnostics', '## Analysis and Invalidation',
           '## Target and ABI Rules', '## Implementation State',
           '## Proof Requirements', '## Open Questions',
           '## Review Checklist']
ordered(aggregate, core)
ordered(aggregate, details)
assert '- [ ]' not in aggregate

def rows(text, start, end):
    section = text.split(start, 1)[1].split(end, 1)[0]
    result = []
    for line in section.splitlines():
        if line.startswith('|'):
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            if cells and cells[0] and not set(cells[0]) <= {'-', ':'}:
                result.append(cells)
    return result[1:]

inputs = rows(aggregate, '### Exact input acceptance matrix', '## Outputs')
outputs = rows(aggregate, '### Exact output handoff matrix',
               '## Adjacent-Stage Contract')
forms = rows(aggregate,
             '## Exhaustive P06 Aggregate-Form Disposition Matrix',
             '## Verification and Publication')
assert (len(inputs), len(outputs), len(forms)) == (9, 7, 43)
assert all(len(row) == 6 and all(row) for row in forms)
counts = Counter(row[2] for row in forms)
assert counts == {'Normalize': 10, 'Preserve': 23, 'Reject': 10}, counts
for row in forms:
    assert row[2] in ('Normalize', 'Preserve', 'Reject'), row
    assert '`' in row[3] or 'no failure' in row[3], row
    assert row[5] == 'absent', row

for phrase in (
    '`PassId::AggregateCanonicalize`',
    'exact immutable B5 / P05 `MemoryCanonical` module wave',
    '`RawVerified` + `TypesLegal` + `ScalarsCanonical` + `CfgCanonical` + '
    '`SsaCanonical` + `MemoryCanonical`',
    'one private whole-module occurrence transaction',
    'Raw+P01+P02+P03+P04+P05+P06 postconditions',
    'Failure rolls back\nthe complete module wave',
    'Old B5 handles remain stale after revision increment',
    'no target layout/lane count realization, physical\noffset/decomposition',
    'B7 accepts only this exact immutable B6 `AggregatesCanonical` wave'):
    assert phrase in aggregate, phrase

expected_families = (
    'canonical array type/value', 'canonical record/struct type/value',
    'canonical union type/value', 'canonical complex value',
    'canonical multivalue/tuple value',
    'canonical fixed semantic vector value',
    'canonical scalable semantic vector value',
    'canonical typed aggregate copy',
    'canonical insert by stable field/index path',
    'canonical extract by stable field/index path',
    'canonical vector insert/extract/splat/shuffle',
    'canonical semantic by-value parameter/result boundary')
form_names = {row[0] for row in forms}
assert set(expected_families) <= form_names

# Read-only accepted boundaries/framework agree with the P06 contract.
assert 'B6 accepts only this exact immutable B5 `MemoryCanonical` wave' in b5
assert ('The input is one immutable whole-module candidate stamped with the exact\n'
        '`ModuleEpoch`, `ModuleRevision`, ordered function-revision digest') in b7
assert 'cumulative properties through `AggregatesCanonical`' in b7
assert 'Every\nP01-P06 postcondition' in b7
assert 'After any revision increment, every old handle is\nstale' in framework
assert ('aggregate` | `Module` | named aggregate/type identities and by-value '
        'boundaries must agree') in pipeline

for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', aggregate):
    assert (aggregate_path.parent / link).resolve().exists(), link
assert [p.name for p in aggregate_path.parent.iterdir()] == ['README.md']
cpp_hits = []
for path in Path('src').rglob('*'):
    if path.suffix in ('.cpp', '.hpp') and (
            'PassId::AggregateCanonicalize' in path.read_text(errors='ignore')):
        cpp_hits.append(path.as_posix())
assert not cpp_hits, cpp_hits

changed = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
expected = {'src/backend/bir/passes/aggregate/README.md', 'todo.md'}
assert changed == expected, (changed, expected)
print('PASS step=6 inputs=9 outputs=7 forms=43 normalize=10 preserve=23 '
      'reject=10 identities=stable transaction=atomic '
      'invalidation=transitive target=excluded b7=exact '
      'implementation=absent scope=2 next=7')
PY
git diff --check
```

- Result:

```text
PASS step=6 inputs=9 outputs=7 forms=43 normalize=10 preserve=23 reject=10 identities=stable transaction=atomic invalidation=transitive target=excluded b7=exact implementation=absent scope=2 next=7
git diff --check: PASS
```
