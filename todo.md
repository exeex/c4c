# Current Packet

Status: Active
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Converge B8 framework, pipeline and Canonical publication

## Just Finished

- Completed plan Step 7 in required order: call graph, exact-key memory-effects
  audit, then B7/P07 intrinsic canonicalization.
- Converged schema-1 module `CallGraph` with exact epoch/module revision and
  ordered full function-revision digest, empty dependencies/options, stable
  function/symbol/call-site IDs, direct/finite-indirect/unknown/external call
  semantics, explicit optional/malformed forms, stale rejection and precise
  invalidation/preservation. P07 is its earliest phase-B mutation consumer.
- Audited read-only `MemoryEffects`: P07 accepts only exact-B6 function-key
  handles. P06 no-op permits unchanged same-key reuse; a changed revision
  requires validator-proven installation under the B6 key or fresh computation.
  Older B4/B5 handles are stale and cannot substitute.
- Converged P07 with ten input and seven output rows plus 38 closed intrinsic/
  helper/call/variadic/asm forms: nine `Normalize`, 15 `Preserve` and 14
  `Reject`. Every row records one result/stable failure and absent implementation.
- Preserved `InlineAsm` as exactly one opaque ordinary-value node with byte-
  exact template/constraints/clobbers and generic def-use. P07 selects no
  helper, target feature, ABI/varargs location or constraint realization.
- Defined one deterministic private whole-module transaction, complete
  rollback, cumulative Raw+P01-P07 postconditions and exact mutation-derived
  invalidation/recompute behavior.
- Defined one immutable frozen ordinal-7 B7 candidate admitted only to B8.
  P07 cannot mint `CanonicalBir`, and no green local report or reconstructed
  candidate can substitute.
- Confirmed both implementations remain absent: each owner directory contains
  only its contract and no checked-in C/C++ definition names the analysis or
  pass. No shared-owner seam remains, so the packet advances to Step 8.

## Suggested Next

- Execute plan Step 8 in exact order: converge pass-framework support, pipeline
  ordering/capability, then audit the shared Canonical verifier gate and request
  a coordinated boundary if verifier text must change.

## Watchouts

- Keep B8 authority split: framework owns transaction/analysis support,
  pipeline owns the exact ordered capability boundary, and verifier owns the
  complete same-revision Canonical gate. Only their final gate mints
  `CanonicalBir`.
- The B7 candidate must retain its exact epoch/module/function digest,
  canonical-v1 plan/options fingerprint, occurrence lineage, ordinal-7 stamp
  and all P01-P07 properties. No stale analysis is a capability.
- Call-graph and P07 implementations are absent. Their matrices are design
  authority, not runtime coverage.

## Proof

- Documentation-only packet. The delegated scope forbids code, tests, builds
  and log edits; canonical regression logs were not touched.
- Exact scope/diff-check, metadata/order/matrices, keys/stale/unknown forms,
  exact memory-effects key handling, P07 dispositions, transaction/invalidation,
  B8 input, links and implementation truth proof:

```bash
python3 - <<'PY'
from collections import Counter
from pathlib import Path
import re
import subprocess

call_path = Path('src/backend/bir/analysis/call_graph/README.md')
intrinsic_path = Path('src/backend/bir/passes/intrinsics/README.md')
call_graph = call_path.read_text()
intrinsics = intrinsic_path.read_text()
effects = Path('src/backend/bir/analysis/memory_effects/README.md').read_text()
aggregate = Path('src/backend/bir/passes/aggregate/README.md').read_text()
framework = Path('src/backend/bir/passes/README.md').read_text()
pipeline = Path('src/backend/bir/pipeline/README.md').read_text()
verify = Path('src/backend/bir/verify/README.md').read_text()

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
                '## Exhaustive P07 Intrinsic/Call/Asm Disposition Matrix',
                '## Verification and Publication',
                '## Failure and Diagnostics',
                '## Analysis and Invalidation',
                '## Target and ABI Rules', '## Implementation State',
                '## Proof Requirements', '## Open Questions',
                '## Review Checklist']
for text, kind, phase in (
        (call_graph, 'analysis',
         'available from verified Raw; B7 / P07 earliest phase-B consumer'),
        (intrinsics, 'pass', 'B7 / P07')):
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
ordered(call_graph, analysis_details)
ordered(intrinsics, pass_details)

def rows(text, start, end):
    section = text.split(start, 1)[1].split(end, 1)[0]
    result = []
    for line in section.splitlines():
        if line.startswith('|'):
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            if cells and cells[0] and not set(cells[0]) <= {'-', ':'}:
                result.append(cells)
    return result[1:]

cin = rows(call_graph, '### Exact descriptor and input-key matrix', '## Outputs')
cout = rows(call_graph, '### Exhaustive call-graph result-family matrix',
            '## Adjacent-Stage Contract')
iin = rows(intrinsics, '### Exact input and dependency matrix', '## Outputs')
iout = rows(intrinsics, '### Exact output handoff matrix',
            '## Adjacent-Stage Contract')
forms = rows(intrinsics,
             '## Exhaustive P07 Intrinsic/Call/Asm Disposition Matrix',
             '## Verification and Publication')
assert tuple(map(len, (cin, cout, iin, iout, forms))) == (9, 11, 10, 7, 38)
assert all(len(row) == 6 and all(row) for row in forms)
counts = Counter(row[2] for row in forms)
assert counts == {'Normalize': 9, 'Preserve': 15, 'Reject': 14}, counts
for row in forms:
    assert row[2] in ('Normalize', 'Preserve', 'Reject'), row
    assert '`' in row[3] or 'no failure' in row[3], row
    assert row[5] == 'absent', row

for phrase in (
    '`AnalysisId::CallGraph`, schema version 1',
    'exact B6 `AggregatesCanonical` wave',
    'ordered digest of every `(FunctionId, FunctionRevision)`',
    'empty ordered `dependencies` set',
    'canonical empty schema-bound `AnalysisOptionsFingerprint`',
    'direct, proven finite-set indirect or stable-reason unknown-indirect',
    'Direct facts are `Known`; absent bodies/calls are explicit `Absent`/empty',
    'returns `StaleAnalysis`; an old handle never rebinds',
    'P07 is its earliest phase-B mutation\nconsumer'):
    assert phrase in call_graph, phrase
for phrase in (
    '`PassId::IntrinsicCanonicalize`',
    'schema-1 module `CallGraph`',
    'complete ordered schema-1 `MemoryEffects`\nhandle set',
    'changed revision requires validator-proven new-key installation or recomputation',
    'one private whole-module occurrence transaction',
    'Raw+P01+P02+P03+P04+P05+P06+P07 postconditions',
    'Failure rolls back the\ncomplete module wave',
    '`InlineAsm` remains exactly one ordinary-value opaque semantic node',
    'Only B8\'s full same-revision Canonical verifier may mint\n`CanonicalBir`'):
    assert phrase in intrinsics, phrase

# Read-only memory-effects and accepted B6/B8 owners agree with the contract.
assert 'complete key binds descriptor/schema/domain, exact epoch/module/function\nrevisions' in effects
assert 'After revision increment old handles are\nstale' in effects
assert 'B7 accepts only this exact immutable B6 `AggregatesCanonical` wave' in aggregate
assert ('Only the pipeline\'s explicit\npublication gate may consume the final '
        'verified stage object') in framework
assert ('successful `P07` candidate with a complete frozen stage stamp') in pipeline
assert ('The `Canonical` profile is the exact cumulative profile of the immutable B7 /\n'
        'P07 output') in verify
assert ('verifier-private token to mint exactly one immutable `CanonicalBir` '
        'carrying') in verify

for path, text in ((call_path, call_graph), (intrinsic_path, intrinsics)):
    for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
        assert (path.parent / link).resolve().exists(), (path, link)
    assert [p.name for p in path.parent.iterdir()] == ['README.md']
cpp_hits = []
for path in Path('src').rglob('*'):
    if path.suffix in ('.cpp', '.hpp'):
        text = path.read_text(errors='ignore')
        if ('AnalysisId::CallGraph' in text or
                'PassId::IntrinsicCanonicalize' in text):
            cpp_hits.append(path.as_posix())
assert not cpp_hits, cpp_hits

changed = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
expected = {
    'src/backend/bir/analysis/call_graph/README.md',
    'src/backend/bir/passes/intrinsics/README.md',
    'todo.md',
}
assert changed == expected, (changed, expected)
print('PASS step=7 call_graph=9/11 intrinsics=10/7 forms=38 normalize=9 '
      'preserve=15 reject=14 keys=exact stale=rejected unknown=explicit '
      'effects=b6-reuse-or-recompute transaction=atomic '
      'invalidation=transitive b8=exact implementation=absent scope=3 next=8')
PY
git diff --check
```

- Result:

```text
PASS step=7 call_graph=9/11 intrinsics=10/7 forms=38 normalize=9 preserve=15 reject=14 keys=exact stale=rejected unknown=explicit effects=b6-reuse-or-recompute transaction=atomic invalidation=transitive b8=exact implementation=absent scope=3 next=8
git diff --check: PASS
```
