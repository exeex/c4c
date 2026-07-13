# Current Packet

Status: Active
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Converge B6 P06 aggregate

## Just Finished

- Completed plan Step 5 in required order: memory effects, provenance, then
  B5/P05 memory canonicalization.
- Converged schema-1 `MemoryEffects` as an empty-dependency, path-independent
  immutable analysis available from verified Raw and freshly requested at the
  exact B4 revision for P05. Stable instruction/object/effect facts distinguish
  known-empty, `Absent`, conservative `Unknown` and malformed failure.
- Converged schema-1 `Provenance` over exact B4 typed SSA with complete
  same-revision CFG, dominance, publication/value-flow and memory-effects
  dependency keys. Stable object/symbol/path/phi/call/escape/alias facts retain
  parallel `EdgeKey` occurrences and never guess an origin.
- Converged P05 to exact matching B4 facts with 37 closed memory/address/
  atomic/effect rows: 12 `Normalize`, 19 `Preserve` and six `Reject`.
  Source-semantic sizes, alignments and address spaces remain exact without
  target layout, ABI/address-mode or helper selection.
- Defined one deterministic private whole-module transaction, complete
  rollback, cumulative Raw+P01+P02+P03+P04+P05 postconditions, exact CFG/SSA
  preservation and mutation-derived transitive invalidation.
- Defined exact B6 acceptance of one immutable B5 `MemoryCanonical` module
  wave. No analysis report, stale handle, partial function prefix or target
  fact can substitute.
- Confirmed all three implementations remain absent: each owner directory has
  only its contract and no checked-in C/C++ definition names either analysis
  or P05. No shared-owner seam remains, so the packet advances to Step 6.

## Suggested Next

- Execute plan Step 6: converge B6/P06 aggregate forms, transaction,
  cumulative postconditions, invalidation and exact B7 acceptance.

## Watchouts

- P06 must preserve P05 semantic address/GEP paths and source-semantic
  attributes without physical layout decomposition or ABI by-value placement.
- Aggregate values and by-value boundaries may normalize only from exact
  semantic type/field/index identity. Do not guess from source spelling, text,
  pointer identity, target layout or legacy records.
- Memory-effects, provenance and P05 implementations are absent; their
  matrices are design authority rather than runtime coverage.

## Proof

- Documentation-only packet. The delegated scope forbids code, tests, builds
  and log edits; canonical regression logs were not touched.
- Exact scope/diff-check, metadata/order/matrices, dependency/stale/unknown
  forms, exhaustive P05 dispositions, transaction/invalidation, target
  exclusion, B6 handoff, links and implementation truth proof:

```bash
python3 - <<'PY'
from collections import Counter
from pathlib import Path
import re
import subprocess

effects_path = Path('src/backend/bir/analysis/memory_effects/README.md')
provenance_path = Path('src/backend/bir/analysis/provenance/README.md')
memory_path = Path('src/backend/bir/passes/memory/README.md')
effects = effects_path.read_text()
provenance = provenance_path.read_text()
memory = memory_path.read_text()
framework = Path('src/backend/bir/analysis/README.md').read_text()
b4 = Path('src/backend/bir/passes/ssa/README.md').read_text()
b6 = Path('src/backend/bir/passes/aggregate/README.md').read_text()
root = Path('src/backend/bir/README.md').read_text()

def ordered(text, headings):
    positions = [text.index(heading) for heading in headings]
    assert positions == sorted(positions), (headings, positions)

core = ['## Purpose', '## Owns', '## Does Not Own', '## Inputs',
        '## Outputs', '## Adjacent-Stage Contract']
analysis_details = ['## Ordered Behavior', '## Invariants',
                    '## Failure and Diagnostics',
                    '## Analysis and Invalidation',
                    '## Target and ABI Rules', '## Implementation State',
                    '## Proof Requirements', '## Open Questions',
                    '## Review Checklist']
pass_details = ['## Ordered Behavior', '## Invariants',
                '## Exhaustive P05 Memory-Form Disposition Matrix',
                '## Verification and Publication',
                '## Failure and Diagnostics',
                '## Analysis and Invalidation',
                '## Target and ABI Rules', '## Implementation State',
                '## Proof Requirements', '## Open Questions',
                '## Review Checklist']
for text, kind, phase in (
        (effects, 'analysis',
         'available from verified Raw; exact-current B4 input for B5 / P05'),
        (provenance, 'analysis',
         'B5 / P05 earliest consumer and later exact-revision address queries'),
        (memory, 'pass', 'B5 / P05')):
    head = '\n'.join(text.splitlines()[:11])
    for item in ('Contract-Status: under-review',
                 'Implementation-Status: absent', f'Kind: {kind}',
                 f'Phase-ID: {phase}' if kind == 'pass'
                 else f'Applies-To: {phase}',
                 'Upstream:', 'Downstream:', 'Owner-Path:',
                 'Last-Reconciled-Commit:'):
        assert item in head, (item, head)
    ordered(text, core)
    assert '- [ ]' not in text
ordered(effects, analysis_details)
ordered(provenance, analysis_details)
ordered(memory, pass_details)

def rows(text, start, end):
    section = text.split(start, 1)[1].split(end, 1)[0]
    result = []
    for line in section.splitlines():
        if line.startswith('|'):
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            if cells and cells[0] and not set(cells[0]) <= {'-', ':'}:
                result.append(cells)
    return result[1:]

ein = rows(effects, '### Exact descriptor and input-key matrix', '## Outputs')
eout = rows(effects, '### Exhaustive memory/effect result-family matrix',
            '## Adjacent-Stage Contract')
pin = rows(provenance, '### Exact descriptor and input-key matrix', '## Outputs')
pout = rows(provenance, '### Exhaustive provenance result-family matrix',
            '## Adjacent-Stage Contract')
minp = rows(memory, '### Exact input and dependency matrix', '## Outputs')
mout = rows(memory, '### Exact output handoff matrix',
            '## Adjacent-Stage Contract')
forms = rows(memory, '## Exhaustive P05 Memory-Form Disposition Matrix',
             '## Verification and Publication')
assert tuple(map(len, (ein, eout, pin, pout, minp, mout, forms))) == (
    9, 11, 10, 11, 9, 6, 37)
assert all(len(row) == 6 and all(row) for row in forms)
counts = Counter(row[2] for row in forms)
assert counts == {'Normalize': 12, 'Preserve': 19, 'Reject': 6}, counts
for row in forms:
    assert row[2] in ('Normalize', 'Preserve', 'Reject'), row
    assert '`' in row[3] or 'no failure' in row[3], row
    assert row[5] == 'absent', row

for phrase in (
    '`AnalysisId::MemoryEffects`, schema version 1',
    'available\nfrom verified Raw BIR',
    'exact B4 `SsaCanonical` revision',
    'empty ordered `dependencies` set',
    'v1 deliberately has no CFG dependency',
    'target-layout key `None`; preparation digest empty',
    '`Unknown` is conservative result data',
    'Source-semantic size, alignment and address-space facts are observed exactly',
    'returns `StaleAnalysis`; an old handle never rebinds'):
    assert phrase in effects, phrase
for phrase in (
    '`AnalysisId::Provenance`, schema version 1',
    'schema-1\n`Cfg`, `Dominance(Dominators)`, `PublicationValueFlow` and '
    '`MemoryEffects`',
    'ordered complete keys `{Cfg, Dominance, PublicationValueFlow, MemoryEffects}`',
    'parallel `EdgeKey`',
    '`Known`, `Absent` and stable-reason `Unknown`',
    'Any dependency invalidation transitively invalidates provenance',
    'target key `None` and empty\npreparation digest'):
    assert phrase in provenance, phrase
for phrase in (
    '`PassId::MemoryCanonicalize`',
    'complete ordered schema-1 `AnalysisId::MemoryEffects` handle set',
    'complete ordered schema-1 `AnalysisId::Provenance` set',
    '`RawVerified` + `TypesLegal` + `ScalarsCanonical` + `CfgCanonical` + '
    '`SsaCanonical`',
    'one private whole-module occurrence transaction',
    'Raw+P01+P02+P03+P04+P05 postconditions',
    'Failure rolls back the entire\nmodule wave',
    'preserves source-semantic sizes, alignments and address spaces exactly',
    'B6 accepts only this exact immutable B5 `MemoryCanonical` wave'):
    assert phrase in memory, phrase

# Read-only root/framework/accepted B4/B6 agree with the converged boundary.
assert 'from Raw BIR before memory/intrinsic normalization' in root
assert 'After any revision increment, every old handle is\nstale' in framework
assert 'B5 accepts only this exact immutable B4 wave with `SsaCanonical`' in b4
assert ('The input is one immutable whole-module candidate stamped with the exact\n'
        '`ModuleEpoch`, `ModuleRevision`, ordered function-revision digest') in b6
assert 'cumulative properties through `MemoryCanonical`' in b6

for path, text in ((effects_path, effects),
                   (provenance_path, provenance), (memory_path, memory)):
    for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
        assert (path.parent / link).resolve().exists(), (path, link)
    assert [p.name for p in path.parent.iterdir()] == ['README.md']
cpp_hits = []
for path in Path('src').rglob('*'):
    if path.suffix in ('.cpp', '.hpp'):
        text = path.read_text(errors='ignore')
        if any(name in text for name in (
                'AnalysisId::MemoryEffects', 'AnalysisId::Provenance',
                'PassId::MemoryCanonicalize')):
            cpp_hits.append(path.as_posix())
assert not cpp_hits, cpp_hits

changed = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
expected = {
    'src/backend/bir/analysis/memory_effects/README.md',
    'src/backend/bir/analysis/provenance/README.md',
    'src/backend/bir/passes/memory/README.md',
    'todo.md',
}
assert changed == expected, (changed, expected)
print('PASS step=5 effects=9/11 provenance=10/11 memory=9/6 forms=37 '
      'normalize=12 preserve=19 reject=6 dependencies=exact stale=rejected '
      'unknown=explicit transaction=atomic invalidation=transitive '
      'target=excluded b6=exact implementation=absent scope=4 next=6')
PY
git diff --check
```

- Result:

```text
PASS step=5 effects=9/11 provenance=10/11 memory=9/6 forms=37 normalize=12 preserve=19 reject=6 dependencies=exact stale=rejected unknown=explicit transaction=atomic invalidation=transitive target=excluded b6=exact implementation=absent scope=4 next=6
git diff --check: PASS
```
