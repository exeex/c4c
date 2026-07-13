# Current Packet

Status: Active
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Converge memory/provenance analyses and B5 P05 memory

## Just Finished

- Completed plan Step 4 in required order: dominance, publication/value-flow,
  then B4/P04 SSA.
- Converged schema-1 `Dominance` over the exact B3 `CfgCanonical` checkpoint
  and freshly recomputed same-revision `Cfg`. Its closed key/dependency/options
  contract preserves stable block/value/instruction and parallel-`EdgeKey`
  dominance/frontier semantics, explicit empty/unreachable/no-exit states,
  stale rejection and transitive invalidation.
- Converged schema-1 `PublicationValueFlow` over the same B3 revision with
  exact CFG and dominance dependencies. Its stable definition/use/parameter/
  return/call/phi/carrier facts distinguish `Known`, `Absent`, reasoned
  `Unknown` and malformed failure without implying BIR publication authority.
- Converged P04 to exact-current B3 analyses with 32 closed SSA-form rows: 11
  `Normalize`, 11 `Preserve` and ten `Reject`. Exact phi incoming coverage
  retains every parallel `EdgeKey`; no name, position, dense index, legacy
  record or unregistered rule may identify or invent a value.
- Defined deterministic full-wave private transactions, complete rollback,
  cumulative Raw+P01+P02+P03+P04 postconditions, typed atomic RAUW, exact
  def-use/phi verification and mutation-derived transitive invalidation.
- Defined exact B5 acceptance of one immutable B4 `SsaCanonical` wave with
  complete def-use and edge-occurrence phi coverage. B3 handles cannot cross a
  changed revision.
- Confirmed all three implementations remain absent: each owner directory has
  only its contract and no checked-in C/C++ definition names either analysis
  or P04 pass. No shared-owner seam remains, so the packet advances to Step 5.

## Suggested Next

- Execute plan Step 5 in required order: converge memory-effects and
  provenance prerequisites, then B5/P05 memory and its B6 handoff.

## Watchouts

- Memory effects may be available from Raw, but every handle used by P05 must
  match the exact B4 revision; provenance requires its exact typed-value/CFG/
  dominance prerequisites. Neither analysis is stored semantic truth.
- Preserve exact CFG/SSA and parallel-edge identity through P05. Do not infer
  alias, object identity, alignment, address decomposition or effect semantics
  from names, text, target layout, ABI/helper routes or compatibility records.
- Dominance, publication/value-flow and P04 implementations are absent; their
  matrices are design authority, not runtime coverage.

## Proof

- Documentation-only packet. The delegated scope forbids code, tests, builds
  and log edits; canonical regression logs were not touched.
- Exact scope/diff-check, metadata/order/matrices, dependency/stale forms,
  parallel-edge dominance/availability, exhaustive P04 dispositions,
  transaction/invalidation, B5 handoff, links and implementation truth proof:

```bash
python3 - <<'PY'
from collections import Counter
from pathlib import Path
import re
import subprocess

dominance_path = Path('src/backend/bir/analysis/dominance/README.md')
publication_path = Path('src/backend/bir/analysis/publication/README.md')
ssa_path = Path('src/backend/bir/passes/ssa/README.md')
dominance = dominance_path.read_text()
publication = publication_path.read_text()
ssa = ssa_path.read_text()
framework = Path('src/backend/bir/analysis/README.md').read_text()
cfg = Path('src/backend/bir/analysis/cfg/README.md').read_text()
b5 = Path('src/backend/bir/passes/memory/README.md').read_text()

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
                '## Exhaustive P04 SSA-Form Disposition Matrix',
                '## Verification and Publication',
                '## Failure and Diagnostics',
                '## Analysis and Invalidation',
                '## Target and ABI Rules', '## Implementation State',
                '## Proof Requirements', '## Open Questions',
                '## Review Checklist']
for text, kind, phase in (
        (dominance, 'analysis',
         'B4 / P04 earliest consumer and later exact-CFG semantic queries'),
        (publication, 'analysis',
         'B4 / P04 earliest consumer and later exact-revision semantic queries'),
        (ssa, 'pass', 'B4 / P04')):
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
ordered(dominance, analysis_details)
ordered(publication, analysis_details)
ordered(ssa, pass_details)

def rows(text, start, end):
    section = text.split(start, 1)[1].split(end, 1)[0]
    result = []
    for line in section.splitlines():
        if line.startswith('|'):
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            if cells and cells[0] and not set(cells[0]) <= {'-', ':'}:
                result.append(cells)
    return result[1:]

din = rows(dominance, '### Exact descriptor and input-key matrix', '## Outputs')
dout = rows(dominance, '### Exhaustive dominance result-family matrix',
            '## Adjacent-Stage Contract')
pin = rows(publication, '### Exact descriptor and input-key matrix', '## Outputs')
pout = rows(publication, '### Exhaustive value-flow result-family matrix',
            '## Adjacent-Stage Contract')
sin = rows(ssa, '### Exact input and dependency matrix', '## Outputs')
sout = rows(ssa, '### Exact output handoff matrix',
            '## Adjacent-Stage Contract')
forms = rows(ssa, '## Exhaustive P04 SSA-Form Disposition Matrix',
             '## Verification and Publication')
assert tuple(map(len, (din, dout, pin, pout, sin, sout, forms))) == (
    9, 9, 9, 10, 9, 6, 32)
assert all(len(row) == 6 and all(row) for row in forms)
counts = Counter(row[2] for row in forms)
assert counts == {'Normalize': 11, 'Preserve': 11, 'Reject': 10}, counts
for row in forms:
    assert row[2] in ('Normalize', 'Preserve', 'Reject'), row
    assert '`' in row[3] or 'no failure' in row[3], row
    assert row[5] == 'absent', row

for phrase in (
    '`AnalysisId::Dominance`, schema version 1',
    'freshly\nrecomputed `AnalysisId::Cfg` handle',
    'ordered singleton `{AnalysisId::Cfg, schema 1, complete key}`',
    '`Dominators` or `DominatorsAndPostdominators`',
    'parallel occurrences remain distinct',
    '`Known`, `Absent`, `Unreachable` and `NoExit`',
    'A key\nmismatch returns `StaleAnalysis`',
    'Any CFG dependency invalidation transitively invalidates dominance'):
    assert phrase in dominance, phrase
for phrase in (
    '`AnalysisId::PublicationValueFlow`, schema 1',
    'schema-1 `AnalysisId::Cfg` at identical complete key',
    'schema-1 `AnalysisId::Dominance(Dominators)`',
    'ordered `{Cfg complete key, Dominance complete key}`',
    'parallel edges remain distinct',
    '`Unknown` is result data and denies a rewrite',
    'emit `Known`,\n   `Absent` or stable-reason `Unknown` facts',
    'Any CFG/dominance invalidation transitively invalidates publication flow'):
    assert phrase in publication, phrase
for phrase in (
    '`PassId::SsaCanonicalize`',
    'exact-current schema-1 `Cfg`,\n`Dominance(Dominators)` and '
    '`PublicationValueFlow` handles',
    '`RawVerified` + `TypesLegal` + `ScalarsCanonical` + `CfgCanonical`',
    'one private transaction per function in the wave',
    'Raw+P01+P02+P03+P04 postconditions',
    'Failure rolls back\nthe entire wave',
    'B5 accepts only this exact immutable B4 wave with `SsaCanonical`',
    'Old B3 handles remain stale after revision increment'):
    assert phrase in ssa, phrase

# Read-only framework, accepted CFG and B5 agree with this boundary.
assert 'After any revision increment, every old handle is\nstale' in framework
assert 'P03 deliberately requires post-mutation CFG recomputation' in cfg
assert ('The input is one immutable whole-module candidate stamped with the exact\n'
        '`ModuleEpoch`, `ModuleRevision`, ordered function-revision digest') in b5
assert 'cumulative properties through `SsaCanonical`' in b5
assert 'exact `EdgeKey` phi coverage' in b5

for path, text in ((dominance_path, dominance),
                   (publication_path, publication), (ssa_path, ssa)):
    for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
        assert (path.parent / link).resolve().exists(), (path, link)
    assert [p.name for p in path.parent.iterdir()] == ['README.md']
cpp_hits = []
for path in Path('src').rglob('*'):
    if path.suffix in ('.cpp', '.hpp'):
        text = path.read_text(errors='ignore')
        if (any(name in text for name in (
                'AnalysisId::Dominance',
                'AnalysisId::PublicationValueFlow',
                'PassId::SsaCanonicalize'))):
            cpp_hits.append(path.as_posix())
assert not cpp_hits, cpp_hits

changed = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
expected = {
    'src/backend/bir/analysis/dominance/README.md',
    'src/backend/bir/analysis/publication/README.md',
    'src/backend/bir/passes/ssa/README.md',
    'todo.md',
}
assert changed == expected, (changed, expected)
print('PASS step=4 dominance=9/9 publication=9/10 ssa=9/6 forms=32 '
      'normalize=11 preserve=11 reject=10 dependencies=exact stale=rejected '
      'parallel_edges=exact transaction=atomic invalidation=transitive '
      'b5=exact implementation=absent scope=4 next=5')
PY
git diff --check
```

- Result:

```text
PASS step=4 dominance=9/9 publication=9/10 ssa=9/6 forms=32 normalize=11 preserve=11 reject=10 dependencies=exact stale=rejected parallel_edges=exact transaction=atomic invalidation=transitive b5=exact implementation=absent scope=4 next=5
git diff --check: PASS
```
