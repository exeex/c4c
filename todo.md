# Current Packet

Status: Complete
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Prove phase-C acceptance and Child-B completion

## Just Finished

- Completed plan Step 9 as a read-only phase-B acceptance audit. All 16 changed
  owners are metadata-complete, core-first, substantive and link-clean: seven
  analyses, seven P01-P07 pass owners, the pass framework and the pipeline.
- Finalized the phase-wide matrix inventory: analysis key/input rows
  `8/8/9/9/9/10/9`, analysis result rows `7/9/9/10/11/11/11`, pass input rows
  `7/8/8/9/9/9/10`, pass output rows `5/5/6/6/6/7/7`, and 226 exhaustive
  form rows partitioned exactly as 81 Normalize, 94 Preserve and 51 Reject.
- Proved exact P01-P07+B8 order, accepted descriptor kinds
  `Module/Function/Function/Function/Module/Module/Module`, seven exact stamp
  axes, cumulative properties, same-owner B7/B8 verification, atomic
  last-good rollback and exact resume behavior.
- Proved every phase-B analysis is an exact-key immutable product requested at
  its pipeline-declared earliest consumer, with explicit dependency keys,
  stale-handle rejection and transitive invalidation; no analysis becomes
  stage or publication authority.
- Closed the cross-document adjacency chain as A2 Raw -> P01 ->
  `ComparisonSelect`/P02 -> `Cfg`/P03 -> fresh `Cfg` plus `Dominance` plus
  `PublicationValueFlow`/P04 -> `MemoryEffects` plus `Provenance`/P05 -> P06 ->
  `CallGraph` plus exact-B6 `MemoryEffects`/P07 -> B8 -> C1. Every edge names
  the matching input property, output property and exact-current analysis key.
- Proved framework, pipeline and verifier authority remains separate. B8 alone
  consumes the same frozen P07 owner and one complete private verifier token
  to mint exactly one immutable `CanonicalBir`; partial/mixed state, cached
  reports and stale analyses cannot publish it.
- Proved the B/C adjacency: C1 accepts only the verifier-gated,
  target-independent, unallocated `CanonicalBir`. No Raw alias, target/profile
  fact, ABI placement, preparation/constraint fact, home, spill, frame,
  allocation or MIR state crosses the handoff or writes backward into it.
- Confirmed implementation truth: all seven analyses and seven semantic passes
  are absent; framework/pipeline remain `partial-foundation`. Documentation
  acceptance does not claim runnable P01-P07/B8 implementation.
- Audited the eight phase-B convergence commits plus the accepted framework
  target-boundary authorization. Scope is documentation/todo/authorized
  lifecycle state only, with no LIR, idea 734, source code, test, build or log
  change and no expectation downgrade, unsupported downgrade or testcase
  overfit.

## Suggested Next

- Ask `c4c-plan-owner` for the required source-idea completion judgment: close
  Child B, deactivate it, or identify one exact remaining documentation seam.
  Do not activate Child C directly from this executor packet.

## Watchouts

- Runbook exhaustion and this green audit do not independently prove or record
  source-idea completion; that lifecycle judgment remains plan-owner-owned.
- Umbrella idea 732 remains open through Children B-F and its final cross-phase
  audit. Phase C remains documentation lifecycle work, not authorization for
  target-aware implementation.
- Canonical pass/analysis implementation is still absent and framework/
  pipeline implementation is only partial foundation.

## Proof

- Documentation-only audit. The delegated packet explicitly forbids code,
  tests, builds and log edits, so no `test_after.log` was created or modified.
- Exact structure/matrix/order/kind/property/analysis/invalidation/rollback/
  authority/handoff/link/implementation-truth/commit-scope proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

A = ('comparison', 'cfg', 'dominance', 'publication', 'memory_effects',
     'provenance', 'call_graph')
P = ('legalize', 'scalar', 'cfg', 'ssa', 'memory', 'aggregate', 'intrinsics')
analyses = {n: Path(f'src/backend/bir/analysis/{n}/README.md') for n in A}
passes = {n: Path(f'src/backend/bir/passes/{n}/README.md') for n in P}
framework = Path('src/backend/bir/passes/README.md')
pipeline = Path('src/backend/bir/pipeline/README.md')
verifier = Path('src/backend/bir/verify/README.md')
root = Path('src/backend/bir/README.md')
idea_b = Path('ideas/open/736_bir_phase_b_canonical_document_convergence.md')
idea_c = Path('ideas/open/737_bir_phase_c_preparation_document_convergence.md')
owners = list(analyses.values()) + list(passes.values()) + [framework, pipeline]
read = {p: p.read_text() for p in owners + [verifier, root, idea_b, idea_c]}

def rows(text, start, end):
    section = text.split(start, 1)[1].split(end, 1)[0]
    found = []
    for line in section.splitlines():
        if line.startswith('|'):
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            if cells and cells[0] and not set(cells[0]) <= {'-', ':'}:
                found.append(cells)
    return found[1:]

core = ('## Purpose', '## Owns', '## Does Not Own', '## Inputs',
        '## Outputs', '## Adjacent-Stage Contract')
for path in owners:
    text = read[path]
    head = '\n'.join(text.splitlines()[:11])
    positions = [text.index(heading) for heading in core]
    assert positions == sorted(positions), (path, positions)
    for item in ('Contract-Status: under-review', 'Upstream:', 'Downstream:',
                 f'Owner-Path: `{path}`', 'Last-Reconciled-Commit:'):
        assert item in head, (path, item)
    assert '- [ ]' not in text, path
    assert all(len(text.split(h, 1)[1].split('\n## ', 1)[0].strip()) > 40
               for h in core), path

analysis_inputs = dict(zip(A, (8, 8, 9, 9, 9, 10, 9)))
analysis_outputs = dict(zip(A, (7, 9, 9, 10, 11, 11, 11)))
earliest = {
    'comparison': 'B2 / P02 earliest consumer',
    'cfg': 'B3 / P03 earliest planning consumer',
    'dominance': 'B4 / P04 earliest consumer',
    'publication': 'B4 / P04 earliest consumer',
    'memory_effects': 'available from verified Raw',
    'provenance': 'B5 / P05 earliest consumer',
    'call_graph': 'available from verified Raw',
}
for name, path in analyses.items():
    text = read[path]
    lower = text.lower()
    head = '\n'.join(text.splitlines()[:11])
    assert 'Implementation-Status: absent' in head and 'Kind: analysis' in head
    assert len(rows(text, '### Exact descriptor and input-key matrix',
                    '## Outputs')) == analysis_inputs[name]
    assert len(rows(text, '## Outputs',
                    '## Adjacent-Stage Contract')) == analysis_outputs[name]
    for item in ('analysisid::', 'schema', 'staleanalysis', 'dependency',
                 'invalidat', 'preparation', 'publishes no partial',
                 'implementation is absent', 'old handle'):
        assert item in lower, (name, item)
    assert ('target-layout key `None`' in text or 'target key `None`' in text
            or 'no target-layout key' in text), name
    assert ('preparation digest empty' in text or 'preparation empty' in text
            or 'empty preparation digest' in text), name
    assert earliest[name] in text, (name, earliest[name])

pass_inputs = dict(zip(P, (7, 8, 8, 9, 9, 9, 10)))
pass_outputs = dict(zip(P, (5, 5, 6, 6, 6, 7, 7)))
form_rows = dict(zip(P, (21, 27, 28, 32, 37, 43, 38)))
dispositions = dict(zip(P, ((11, 7, 3), (14, 10, 3), (14, 9, 5),
                            (11, 11, 10), (12, 19, 6), (10, 23, 10),
                            (9, 15, 14))))
property_edges = dict(zip(P, (
    ('RawBir', 'TypesLegal'),
    ('TypesLegal', 'ScalarsCanonical'),
    ('ScalarsCanonical', 'CfgCanonical'),
    ('CfgCanonical', 'SsaCanonical'),
    ('SsaCanonical', 'MemoryCanonical'),
    ('MemoryCanonical', 'AggregatesCanonical'),
    ('AggregatesCanonical', 'IntrinsicsCanonical'),
)))
for ordinal, (name, path) in enumerate(passes.items()):
    text = read[path]
    lower = text.lower()
    head = '\n'.join(text.splitlines()[:11])
    assert 'Implementation-Status: absent' in head and 'Kind: pass' in head
    matrices = list(re.finditer(r'^### Exact .*? matrix$', text, re.M))
    assert len(matrices) == 2, name
    matrix_lengths = []
    for matrix in matrices:
        end = text.find('\n## ', matrix.end())
        matrix_lengths.append(len([line for line in
                                   text[matrix.end():end].splitlines()
                                   if line.startswith('|')]) - 2)
    assert tuple(matrix_lengths) == (pass_inputs[name], pass_outputs[name])
    section = re.search(r'^## Exhaustive .*? Matrix\n(.*?)(?=^## Verification)',
                        text, re.M | re.S).group(1)
    forms = [line for line in section.splitlines() if line.startswith('|')][2:]
    counts = tuple(sum(f'| {kind} |' in line for line in forms)
                   for kind in ('Normalize', 'Preserve', 'Reject'))
    assert (len(forms), counts) == (form_rows[name], dispositions[name])
    for prop in property_edges[name]:
        assert prop in text, (name, prop)
    for item in ('occurrence', 'rollback', 'publishes no', 'invalidat',
                 'target-independent', 'implementation is absent', 'partial'):
        assert item in lower, (name, item)

framework_text = read[framework]
pipeline_text = read[pipeline]
verify_text = read[verifier]
for text, kind in ((framework_text, 'framework'), (pipeline_text, 'pipeline')):
    metadata = text.split('## Purpose', 1)[0]
    assert f'Kind: {kind}' in metadata
    assert 'Implementation-Status: partial-foundation' in metadata
framework_counts = (
    len(rows(framework_text, '### Exact framework input matrix', '## Outputs')),
    len(rows(framework_text, '### Exact framework output matrix',
             '## Adjacent-Stage Contract')),
)
descriptors = rows(framework_text, '## Closed Canonical Descriptor Matrix',
                   '## Ordered Behavior')
pipeline_counts = (
    len(rows(pipeline_text, '### Exact pipeline input matrix', '## Outputs')),
    len(rows(pipeline_text, '### Exact pipeline output matrix',
             '## Adjacent-Stage Contract')),
)
sequence = rows(pipeline_text, '## Exact Built-In Occurrence Sequence',
                '## Stage Stamp and Fingerprint Matrix')
stamps = rows(pipeline_text, '## Stage Stamp and Fingerprint Matrix',
             '## Ordered Behavior')
assert framework_counts == (8, 8) and len(descriptors) == 7
assert pipeline_counts == (8, 6) and len(sequence) == 8 and len(stamps) == 7
assert [row[0] for row in sequence] == [
    'B1 / P01', 'B2 / P02', 'B3 / P03', 'B4 / P04',
    'B5 / P05', 'B6 / P06', 'B7 / P07', 'B8']
assert [row[1] for row in descriptors] == [
    'Module', 'Function', 'Function', 'Function', 'Module', 'Module', 'Module']
for expected, row in zip(('none', 'ComparisonSelect', 'Cfg', 'Dominance',
                          'MemoryEffects', 'recompute/invalidate', 'CallGraph',
                          'not publication capability'), sequence):
    assert expected in row[4], (expected, row[4])
for item in ('cumulative properties', 'last-good', 'rollback',
             'Resume starts strictly', 'always runs B8',
             'partial foundation only'):
    assert item in pipeline_text, item

for item in (
    'framework | choose P01-P07 order, define Canonical rules, mint `CanonicalBir`',
    'pipeline | define pass semantics or declare verifier success',
    'verifier | schedule passes, mutate candidates or create pipeline checkpoints',
    'same candidate'):
    assert item in pipeline_text, item
verify_flat = ' '.join(verify_text.split())
for item in ('same owning revision frozen by P07', 'stale analysis result',
             'verifier-private token to mint exactly one immutable `CanonicalBir`',
             'publishes no function subset', 'No earlier green report'):
    assert item in verify_flat, item

root_text = read[root]
idea_b_text = read[idea_b]
idea_c_text = read[idea_c]
for item in ('`B8` | Canonical verification and publication',
             'verified, target-independent, unallocated `CanonicalBir`',
             '`C1` | `TargetProfile` selection and validation | `CanonicalBir`'):
    assert item in root_text, item
for item in ('phase C receives exactly verified',
             'target-independent `CanonicalBir`', 'no Raw aliases, target facts',
             'allocation state, stale analysis'):
    assert item in idea_b_text, item
for item in ('Predecessor: accepted `ideas/open/736_bir_phase_b_canonical_document_convergence.md`',
             'without mutating `CanonicalBir`',
             'never target facts\n  written backward into Canonical storage'):
    assert item in idea_c_text, item
for item in ('one move-only `CanonicalBir`', 'No target/profile/',
             'layout/ABI/helper/preparation/constraint/home/spill/frame/MIR fact appears'):
    assert item in pipeline_text, item
for item in ('Raw and Canonical BIR carry no semantic `target_profile`',
             'Neither decision is imported into Raw or Canonical\nstorage'):
    assert item in verify_text, item

for path in owners:
    for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', read[path]):
        if '://' in link or link.startswith('#'):
            continue
        assert (path.parent / link.split('#', 1)[0]).resolve().exists(), (path, link)

commits = ('c7bb43d3e', 'ce3dacf3f', '878e56a97', '5eb4d6f43',
           'b6cabf1d2', 'd4c73bdf4', 'c0e3cdc6e', '938c7b43e', '7b01fd0eb')
changed = []
for commit in commits:
    files = subprocess.check_output(
        ['git', 'show', '--format=', '--name-only', commit], text=True).split()
    changed.extend(files)
    for file in files:
        assert not file.startswith('src/backend/bir/lir_to_bir/'), (commit, file)
        assert '734_' not in file, (commit, file)
        assert not (file.startswith('src/') and Path(file).suffix in
                    {'.cpp', '.cc', '.c', '.hpp', '.h', '.rs'}), (commit, file)
        assert not (file.startswith('test') or '/test' in file or
                    file.startswith('build') or file.endswith('.log')), (commit, file)
allowed = ({'todo.md', 'plan.md',
            'ideas/open/736_bir_phase_b_canonical_document_convergence.md'} |
           {str(path) for path in owners})
assert set(changed) <= allowed, set(changed) - allowed
for reject_signal in ('Named-test shortcuts', 'rendered-text probes',
                      'unsupported\n  downgrades', 'expected-output weakening'):
    assert reject_signal in idea_b_text, reject_signal

status = subprocess.check_output(['git', 'status', '--short'], text=True)
assert status == ' M todo.md\n', status
print('PASS step=9 owners=16 analyses=7 passes=7 framework=1 pipeline=1 '
      'analysis_io=62/68 pass_io=60/42 forms=226 dispositions=81/94/51 '
      'framework=8/8/7 pipeline=8/6 order=P01-P07-B8 '
      'kinds=M,F,F,F,M,M,M stamps=7 rollback=atomic '
      'analyses=earliest-exact-current authority=separate '
      'canonical=one-verifier-gated '
      'phase_c=canonical-only-no-raw-or-target-abi-home-spill-allocation '
      'implementation=14-absent+2-partial-foundation links=16 '
      'scope=9-commits forbidden_edits=0 overfit=0')
PY
```

- Result:

```text
PASS step=9 owners=16 analyses=7 passes=7 framework=1 pipeline=1 analysis_io=62/68 pass_io=60/42 forms=226 dispositions=81/94/51 framework=8/8/7 pipeline=8/6 order=P01-P07-B8 kinds=M,F,F,F,M,M,M stamps=7 rollback=atomic analyses=earliest-exact-current authority=separate canonical=one-verifier-gated phase_c=canonical-only-no-raw-or-target-abi-home-spill-allocation implementation=14-absent+2-partial-foundation links=16 scope=9-commits forbidden_edits=0 overfit=0
```
