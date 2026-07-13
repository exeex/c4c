# Current Packet

Status: Active
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Converge comparison analysis and B2 P02 scalar

## Just Finished

- Completed plan Step 1 under coordinated authorization `c7bb43d3e`.
- Replaced only the contradictory paragraph in
  `src/backend/bir/passes/README.md` **Allowed dependency direction**. The
  framework now distinguishes accepted source-semantic typed sizes,
  alignments and address spaces from excluded semantic `target_profile`,
  rendered `data_layout`, target triple, pointer-width/address-space layout
  selection and all other C1/C2 context. C1 selects the exact `TargetProfile`;
  C2 derives target layout. Every other framework/B8 clause is unchanged.
- Converged `passes/legalize/README.md` to the uniform metadata spine,
  core-first ownership/input/output/adjacency order and applicable detail
  order. P01 consumes only one move-only immutable verified,
  target-independent, unallocated exact-revision `RawBir` and cannot repair
  inherited A2 failure.
- Added exact input and output handoff matrices plus a closed exhaustive P01
  form matrix: 11 `Normalize`, seven `Preserve` and three `Reject` rows. Every
  row names lossless facts, one disposition, exact result and stable failure,
  next owner/B2 visibility and truthful absent implementation status.
- Defined deterministic full-module inventory, one private exact-revision
  transaction, mutation-derived revision/stamp and `MutationSummary`, complete
  rollback, cumulative Raw+P01 postconditions, framework-only `TypesLegal`
  establishment and mutation-derived analysis invalidation. B2 accepts only
  that exact immutable P01 output.
- Re-audited the accepted Raw boundary after the narrow repair. No remaining
  shared-owner seam, target/ABI/preparation authority, implementation claim or
  Step-1 completion gap was found, so the active packet advances to Step 2.

## Suggested Next

- Execute plan Step 2 in required order: converge the exact-revision immutable
  comparison/select analysis first, then make B2/P02 consume the matching P01
  `TypesLegal` revision and analysis result while owning only portable scalar,
  comparison, cast and select normalization.

## Watchouts

- The early framework edit was only the authorized Raw/P01 adjacency repair;
  full pass-framework and B8 convergence remains Step 8.
- Preserve P01's distinction between source-semantic typed attributes and
  target layout. Step 2 cannot introduce target/profile/helper/ABI authority or
  reinterpret opaque inline-asm payload.
- P01 implementation is absent. The matrices are design authority, not runtime
  coverage; no valid form may be called complete through rejection alone.

## Proof

- Docs-only packet: no canonical regression log applies and neither
  `test_before.log` nor `test_after.log` changed.
- Minimal framework-clause diff, metadata/core/matrix/order, accepted Raw
  boundary, transaction/invalidation/B2 handoff, implementation truth and
  exact scope proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

framework_path = Path('src/backend/bir/passes/README.md')
legalize_path = Path('src/backend/bir/passes/legalize/README.md')
core = Path('src/backend/bir/core/README.md').read_text()
verify = Path('src/backend/bir/verify/README.md').read_text()
scalar = Path('src/backend/bir/passes/scalar/README.md').read_text()
framework = framework_path.read_text()
legalize = legalize_path.read_text()

# The shared repair is exactly one paragraph/hunk; every other B8/framework
# clause stays byte-identical to HEAD.
diff = subprocess.check_output(
    ['git', 'diff', '--unified=0', 'HEAD', '--', framework_path.as_posix()],
    text=True)
assert diff.count('@@') == 2, diff  # one hunk has opening and closing @@
removed = [line[1:] for line in diff.splitlines()
           if line.startswith('-') and not line.startswith('---')]
added = [line[1:] for line in diff.splitlines()
         if line.startswith('+') and not line.startswith('+++')]
assert len(removed) == 4 and len(added) == 7, (removed, added)
assert removed == [
    'Target-independent BIR semantics may include an already-resolved module data',
    'layout where the core schema requires it. That is not permission for a canonical',
    'pass to inspect a target register profile, calling convention placement policy,',
    'legal machine opcode set, or backend feature switch.',
]
assert 'Raw and Canonical BIR may preserve source-semantic typed sizes, alignments, and' in framework
assert '`target_profile`, rendered `data_layout`, target triple, pointer-width/address-' in framework
assert 'the exact `TargetProfile`, and C2 derives target layout' in framework

# Re-audit the accepted Raw side against the repaired framework.
assert '`target_profile` and rendered `data_layout` have no semantic core destination' in core
assert ('Raw and Canonical BIR carry no semantic `target_profile`, target triple,\n'
        'rendered `data_layout`, language-ABI mode, pointer-width/address-space layout\n'
        'selection, or other C1/C2 target context') in verify
assert 'already-resolved module data\nlayout where the core schema requires it' not in framework

spine = [
    'Contract-Status: under-review', 'Implementation-Status: absent',
    'Kind: pass', 'Phase-ID: B1 / P01', 'Upstream:', 'Downstream:',
    'Owner-Path:', 'Last-Reconciled-Commit:',
]
head = '\n'.join(legalize.splitlines()[:11])
assert all(item in head for item in spine), head
core_first = ['## Purpose', '## Owns', '## Does Not Own', '## Inputs',
              '## Outputs', '## Adjacent-Stage Contract']
details = ['## Ordered Behavior', '## Invariants',
           '## Verification and Publication', '## Failure and Diagnostics',
           '## Analysis and Invalidation', '## Target and ABI Rules',
           '## Implementation State', '## Proof Requirements',
           '## Open Questions', '## Review Checklist']
for headings in (core_first, details):
    positions = [legalize.index(heading) for heading in headings]
    assert positions == sorted(positions), (headings, positions)
assert '- [ ]' not in legalize

def table_rows(start, end, pattern):
    section = legalize.split(start, 1)[1].split(end, 1)[0]
    result = []
    for line in section.splitlines():
        match = re.match(pattern, line)
        if match:
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            result.append((match.group(1), cells))
    return result

inputs = table_rows('### Exact input acceptance matrix', '## Outputs',
                    r'^\| ([^|]+) \|')
outputs = table_rows('### Exact output handoff matrix',
                     '## Adjacent-Stage Contract', r'^\| ([^|]+) \|')
forms = table_rows('## Exhaustive P01 Raw-Form Disposition Matrix',
                   '## Verification and Publication', r'^\| ([^|]+) \|')
inputs = [(name, cells) for name, cells in inputs
          if name.strip() not in ('Input axis', '---')]
outputs = [(name, cells) for name, cells in outputs
           if name.strip() not in ('Output/product', '---')]
forms = [(name, cells) for name, cells in forms
         if len(cells) == 6 and cells[2] in ('Normalize', 'Preserve', 'Reject')]
assert len(inputs) == 7 and len(outputs) == 5, (len(inputs), len(outputs))
expected_forms = [
    'integer literal with noncanonical storage padding',
    'floating literal with noncanonical padding/encoding',
    'portable type alias or spelling-only signedness form',
    'admitted scalar opcode alias', 'admitted comparison predicate alias',
    'admitted cast alias', 'non-`i1` scalar truth boundary',
    'arbitrary-width integer semantic operation',
    'extended-floating semantic operation', 'complex semantic operation',
    'checked/overflow arithmetic',
    'switch/indirect successor and other valid CFG form',
    'phi/forward-reference form already resolved by A2',
    'address, memory, stack and atomic semantic form',
    'aggregate/by-value semantic form', 'registered intrinsic semantic form',
    '`InlineAsm` ordinary semantic node',
    'source-semantic typed object/type attributes',
    'valid form with no lossless portable representation',
    'valid form with no declared P01/later owner',
    'unknown/unlisted applicable Raw-only form',
]
assert [name.strip() for name, _ in forms] == expected_forms
counts = {'Normalize': 0, 'Preserve': 0, 'Reject': 0}
for name, cells in forms:
    assert len(cells) == 6 and all(cells), (name, cells)
    disposition = cells[2]
    assert disposition in counts, (name, disposition)
    counts[disposition] += 1
    assert '`' in cells[3], (name, cells[3])  # stable failure is explicit
    assert cells[5] == 'absent', (name, cells[5])
assert counts == {'Normalize': 11, 'Preserve': 7, 'Reject': 3}, counts

required = [
    'one move-only immutable, verified, target-independent and\nunallocated published `RawBir`',
    '`{ModuleEpoch, ModuleRevision}`',
    'ordered `(FunctionId, FunctionRevision)`\ndigest',
    'one private module occurrence transaction',
    'rolls back and discards the\n   entire candidate',
    'complete `MutationSummary`',
    'accumulated `RawVerified` and newly established\n`TypesLegal`',
    'Failure publishes\nno revision, property, partial rewrite, analysis result/cache entry',
    'invalidates every cached result whose declared axes observe changed constants',
    'B2 and its comparison analysis must request facts\nwhose complete key matches the exact P01 output revision',
    'Implementation is absent',
]
for phrase in required:
    assert phrase in legalize, phrase
assert ('The input is one immutable BIR view with exact epoch, module revision, observed\n'
        'function revisions, and `PassProperty::TypesLegal`') in scalar
assert 'The P01 postcondition and\nconfigured input verifier must accept that same revision' in scalar

# No P01 implementation/build claim exists: the directory contains only the
# contract and no checked-in C/C++ definition names PassId::Legalize.
assert [path.name for path in legalize_path.parent.iterdir()] == ['README.md']
cpp_hits = []
for path in Path('src').rglob('*'):
    if path.suffix in ('.cpp', '.hpp') and 'PassId::Legalize' in path.read_text(
            errors='ignore'):
        cpp_hits.append(path.as_posix())
assert not cpp_hits, cpp_hits

changed = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
expected = {
    'src/backend/bir/passes/README.md',
    'src/backend/bir/passes/legalize/README.md',
    'todo.md',
}
assert changed == expected, (changed, expected)
print('PASS step=1 framework_hunks=1 framework_lines=4->7 inputs=7 outputs=5 '
      'forms=21 normalize=11 preserve=7 reject=3 transaction=atomic '
      'invalidation=mutation-derived b2=exact implementation=absent scope=3 next=2')
PY
git diff --check
```

- Result:

```text
PASS step=1 framework_hunks=1 framework_lines=4->7 inputs=7 outputs=5 forms=21 normalize=11 preserve=7 reject=3 transaction=atomic invalidation=mutation-derived b2=exact implementation=absent scope=3 next=2
git diff --check: PASS
```
