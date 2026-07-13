# Current Packet

Status: Active
Runbook State: Steps Exhausted — Awaiting Plan-Owner Judgment
Source Idea Path: ideas/open/735_bir_phase_a_import_raw_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove phase-B acceptance and the deferred idea-734 boundary

## Just Finished

- Completed plan Step 5 as a read-only final phase-A documentation and
  adjacency proof. All five runbook steps are exhausted; this execution result
  does not itself close Child A or activate Child B or idea 734.
- Mechanically matched the importer and core matrices to the current source in
  exact order at 38 `LirInst` alternatives, six `LirTerminator` alternatives,
  and 18 metadata families. Every importer row has authority, destination or
  non-destination, current disposition, A2 failure rule, and positive plus
  malformed/neighbor proof; every core row has one owner/non-destination,
  truthful implementation disposition and invariant.
- Proved the only phase-A handoff is one private exact-revision move-only
  `ModuleDraft` consumed by the sole full A2
  `verify_and_publish_raw(ModuleDraft&&)` gate into one move-only verified,
  target-independent and unallocated `RawBir`. B1 accepts only its immutable
  exact published revision; no importer map, unresolved fixup, hidden side
  table, unsupported valid row, target context, allocation fact or partial
  capability crosses the boundary.
- Rechecked metadata spines, core/detail heading order, checked review lists,
  relative links, implementation truth, the repaired shared A2 rules and all
  Child-A acceptance/reject signals. No stale unchecked obligation or
  unresolved documentation seam remains.
- History proves Child-A execution changed only its authorized documents and
  lifecycle state. Current LIR and idea 734 were unchanged. Idea 734 remains
  open, deferred and inactive; it can receive implementation authority only
  through a later lifecycle decision after Child-A acceptance. Child B also
  remains open and inactive.

### Final phase-A acceptance matrix

| Acceptance boundary | Result | Exact evidence |
|---|---|---|
| A1 current-LIR intake | Pass | Source and importer rows are equal in exact order at 38 instructions and six terminators; the maintained metadata set is exactly 18. All six importer row fields are nonempty and carry authority, destination/import rule, truthful disposition, A2 failure, and `+`/`-`/`N` proof obligations. |
| Memory subordinate | Pass | Exactly 11 current instruction rows and 11 applicable metadata rows bind by same name to one parent row/family and one core owner. It owns no dispatcher, storage, verifier, finish or publication path; every failure poisons the parent transaction. |
| A2 core receiving | Pass | Core rows equal the source 38/6 order and the importer metadata set. Every row has one nonempty typed owner/non-destination, one checked-in `missing`/`partial`/`checked-in`/validation disposition and one invariant. Load/store/binary/cast/compare/call/GEP/select/inline-asm twins converge on the same semantic owner. |
| Target-independent boundary | Pass | `target_profile` and rendered `data_layout` are validation/origin/parity-only non-destinations. Raw/Canonical contain no semantic C1/C2 target context; C1 independently selects the exact `TargetProfile`, C2 derives layout, and ABI/allocation/MIR facts remain excluded. |
| A1 → A2 publication | Pass | A1 finishes one private move-only exact-revision `ModuleDraft`; the shared verifier alone consumes it through the full Raw registry. Diagnostic checks, builders, constructors and compatibility evidence cannot mint a stage token. Every failure destroys unpublished state and publishes no subset/capability. |
| A2 → B1 / Child B | Pass | A2 produces one move-only verified target-independent/unallocated `RawBir`. B1's only input is the same immutable published exact revision with `RawVerified`; B1 cannot repair A2 failure. Child B names accepted Child A as predecessor and converts that one Raw input through P01-P07/B8 without changing phase-A authority. |
| Forbidden escape/duplicate authority | Pass | Importer maps, fixups, partial drafts/functions, unsupported placeholders/valid rows, names/rendered text, compatibility side tables, target/profile/layout semantics, allocation state and stale analyses are explicitly rejected at the handoff. Terminators alone own CFG successors and ordinary values use exact def-use. |
| Shared post-repair audit | Pass | Commit `65a20c2d` preserves the sole A2 gate, exact revision and failure atomicity while aligning verifier intake, compatibility/legacy matrix deference, B1 inherited-A2 failure and root idea-731 history. Analysis, diagnostics and review remain read-only and non-authoritative. |
| Implementation truth | Pass | Importer/core are `partial`; the memory sub-boundary is `absent`. The documents name the bounded checked-in foundation and missing containers/wiring/full gate separately; unsupported valid rows and build-excluded design do not count as implementation. |
| Document conformance | Pass | All three owners contain the exact metadata spine, core-first and applicable detail headings in order, complete input/output clauses, resolved local links and fully checked review lists. Their open-question sections authorize no work and record no live seam. |
| Lifecycle and history | Pass | Commits `10d70b872`, `060a32c78`, `e759322a`, `237afcdf`, `ba1dcab88`, and `65a20c2d` touch only their authorized phase-A docs/shared repair/lifecycle files. No `src/codegen/lir/**` or idea-734 change occurs from Child-A activation through `HEAD`. Ideas 734, 735, 736 and umbrella 732 remain open as recorded; no downstream activation occurred. |
| Source acceptance/reject signals | Pass | Documentation proves exhaustive ownership, exact adjacency, target independence, failure atomicity, truthful implementation status and deferred implementation authority without code/tests/build edits, LIR edits, text identity, source-gap redirection, expectation weakening, partial publication or activation. |

## Suggested Next

- Ask `c4c-plan-owner` for the required runbook-completion judgment: decide
  from Child A's source acceptance criteria and this supervisor-selected proof
  whether to close Child A, deactivate/replace the exhausted runbook, or record
  a genuinely new bounded requirement. Do not activate Child B or idea 734 as
  part of this executor packet.

## Watchouts

- Runbook exhaustion is evidence for, not authority to perform, lifecycle
  closure. Child A remains open until plan-owner acts.
- The strict umbrella order permits Child B only after accepted Child A; idea
  734 remains a deferred implementation consumer and requires its own later
  lifecycle decision. Neither follows automatically from this proof.
- The documented missing new-BIR containers/wiring/full gate are truthful
  deferred implementation gaps, not unresolved phase-A documentation gaps and
  not permission to edit current LIR.

## Proof

- Docs-only proof; the delegated packet explicitly requires no canonical
  regression logs. Neither `test_before.log` nor `test_after.log` changed.
- Exact inventory/row-field/owner, subordinate uniqueness, handoff,
  conformance/link, shared-rule, history/lifecycle, scope and whitespace proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

source = Path('src/codegen/lir/ir.hpp').read_text()
importer_path = Path('src/backend/bir/lir_to_bir/README.md')
memory_path = Path('src/backend/bir/lir_to_bir/memory/README.md')
core_path = Path('src/backend/bir/core/README.md')
verify_path = Path('src/backend/bir/verify/README.md')
legalize_path = Path('src/backend/bir/passes/legalize/README.md')
child_a_path = Path('ideas/open/735_bir_phase_a_import_raw_document_convergence.md')
child_b_path = Path('ideas/open/736_bir_phase_b_canonical_document_convergence.md')
idea734_path = Path('ideas/open/734_lir_to_new_bir_container_completeness.md')
umbrella_path = Path('ideas/open/732_bir_stage_document_convergence_umbrella.md')

importer = importer_path.read_text()
memory = memory_path.read_text()
core = core_path.read_text()
verify = verify_path.read_text()
legalize = legalize_path.read_text()
child_a = child_a_path.read_text()
child_b = child_b_path.read_text()
idea734 = idea734_path.read_text()
umbrella = umbrella_path.read_text()

def source_variants(alias):
    match = re.search(rf'using {alias} = std::variant<(.*?)>;', source, re.S)
    assert match, alias
    return re.findall(r'\bLir[A-Za-z0-9_]+\b', match.group(1))

def section(text, start, end):
    return text.split(start, 1)[1].split(end, 1)[0]

def rows(text, start, end, pattern):
    result = []
    for line in section(text, start, end).splitlines():
        match = re.match(pattern, line)
        if match:
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            result.append((match.group(1), cells))
    return result

source_inst = source_variants('LirInst')
source_term = source_variants('LirTerminator')
assert len(source_inst) == 38 and len(source_term) == 6

import_inst = rows(importer, '## Exhaustive Instruction Input Matrix',
                   '## Exhaustive Terminator Input Matrix',
                   r'^\| `LirInst::(Lir[A-Za-z0-9_]+)` \|')
import_term = rows(importer, '## Exhaustive Terminator Input Matrix',
                   '## Exhaustive Metadata-Family Input Matrix',
                   r'^\| `LirTerminator::(Lir[A-Za-z0-9_]+)` \|')
core_inst = rows(core, '## Exhaustive Instruction Receiving Matrix',
                 '## Exhaustive Terminator Receiving Matrix',
                 r'^\| `LirInst::(Lir[A-Za-z0-9_]+)` \|')
core_term = rows(core, '## Exhaustive Terminator Receiving Matrix',
                 '## Exhaustive Metadata-Family Receiving Matrix',
                 r'^\| `LirTerminator::(Lir[A-Za-z0-9_]+)` \|')
assert [name for name, _ in import_inst] == source_inst
assert [name for name, _ in import_term] == source_term
assert [name for name, _ in core_inst] == source_inst
assert [name for name, _ in core_term] == source_term

metadata = [
    'module-context', 'stable-identities', 'operand-kinds', 'type-system',
    'functions-signatures', 'blocks-cfg-order', 'values-def-use',
    'stack-objects-allocas', 'globals-objects', 'initializers', 'strings',
    'externs', 'specializations', 'intrinsic-requirements',
    'inline-asm-metadata', 'producer-indexes-caches', 'source-order-origin',
    'module-publication',
]
import_meta = rows(importer, '## Exhaustive Metadata-Family Input Matrix',
                   '## Ordered Behavior', r'^\| `([a-z][a-z0-9-]+)` \|')
core_meta = rows(core, '## Exhaustive Metadata-Family Receiving Matrix',
                 '## Ordered Behavior', r'^\| `([a-z][a-z0-9-]+)` \|')
assert [name for name, _ in import_meta] == metadata
assert [name for name, _ in core_meta] == metadata

for name, cells in import_inst + import_term + import_meta:
    assert len(cells) == 6 and all(cells), (name, cells)
    assert any(mark in cells[1] for mark in ('S:', 'C:', 'V:', 'S current',
                                              'S payload', 'S/V:')), name
    assert any(word in cells[3] for word in
               ('missing', 'partial', 'proved', 'coverage', 'stale',
                'non-destination')), (name, cells[3])
    assert any(term in cells[4] for term in
               ('reject', 'fail', 'cannot', 'no successor/fallthrough')), (name, cells[4])
    assert all(mark in cells[5] for mark in ('+', '-', 'N ')), (name, cells[5])

for name, cells in core_inst + core_term + core_meta:
    assert len(cells) == 4 and all(cells), (name, cells)
    assert any(word in cells[2] for word in
               ('missing', 'partial', 'checked-in', 'validation-only')), (name, cells[2])

inst_owners = {name: cells[1] for name, cells in core_inst}
twins = {
    ('LirLoad', 'LirLoadOp'): 'Load',
    ('LirStore', 'LirStoreOp'): 'Store',
    ('LirBinary', 'LirBinOp'): 'binary/unary',
    ('LirCast', 'LirCastOp'): 'Cast',
    ('LirCmp', 'LirCmpOp'): 'Compare',
    ('LirCall', 'LirCallOp'): 'Call',
    ('LirGep', 'LirGepOp'): 'GetElementPtr',
    ('LirSelect', 'LirSelectOp'): 'Select',
    ('LirInlineAsm', 'LirInlineAsmOp'): 'InlineAsm',
}
for pair, token in twins.items():
    assert all(token in inst_owners[name] for name in pair), (pair, token)

memory_inst = rows(memory, '### Input handoff matrix', '## Outputs',
                   r'^\| `(LirInst::Lir[A-Za-z0-9_]+)` \|')
memory_meta = rows(memory, '### Input handoff matrix', '## Outputs',
                   r'^\| `metadata::([a-z][a-z0-9-]+)` \|')
expected_memory_inst = [
    'LirInst::LirLoad', 'LirInst::LirStore', 'LirInst::LirGep',
    'LirInst::LirMemcpyOp', 'LirInst::LirStackSaveOp',
    'LirInst::LirStackRestoreOp', 'LirInst::LirLoadOp',
    'LirInst::LirStoreOp', 'LirInst::LirMemsetOp', 'LirInst::LirGepOp',
    'LirInst::LirAllocaOp',
]
expected_memory_meta = [
    'stack-objects-allocas', 'stable-identities', 'operand-kinds',
    'type-system', 'values-def-use', 'globals-objects', 'initializers',
    'strings', 'intrinsic-requirements', 'source-order-origin',
    'module-publication',
]
assert [name for name, _ in memory_inst] == expected_memory_inst
assert [name for name, _ in memory_meta] == expected_memory_meta
for name, cells in memory_inst:
    assert len(cells) == 5 and f'Parent row `{name}`' in cells[2], (name, cells)
    assert any(term in cells[4] for term in ('reject', 'inject')) and 'prove' in cells[4], (name, cells[4])
for name, cells in memory_meta:
    assert len(cells) == 5 and f'Parent family `{name}`' in cells[2], (name, cells)
    assert any(term in cells[4] for term in ('reject', 'inject')) and 'prove' in cells[4], (name, cells[4])
for phrase in (
    "parent importer's closed 38/6 dispatcher", 'core storage, builders, the full A2 verifier',
    'owns no verifier profile', 'cannot finish, freeze, return, verify, or publish the draft',
):
    assert phrase in memory, phrase

spine = ['Contract-Status:', 'Implementation-Status:', 'Kind:', 'Phase-ID:',
         'Upstream:', 'Downstream:', 'Owner-Path:', 'Last-Reconciled-Commit:']
core_first = ['## Purpose', '## Owns', '## Does Not Own', '## Inputs',
              '## Outputs', '## Adjacent-Stage Contract']
details = ['## Ordered Behavior', '## Invariants',
           '## Verification and Publication', '## Failure and Diagnostics',
           '## Analysis and Invalidation', '## Target and ABI Rules',
           '## Implementation State', '## Proof Requirements',
           '## Open Questions', '## Review Checklist']
for text in (importer, memory, core):
    assert all(key in '\n'.join(text.splitlines()[:11]) for key in spine)
    assert [text.index(heading) for heading in core_first] == sorted(
        text.index(heading) for heading in core_first)
    assert [text.index(heading) for heading in details] == sorted(
        text.index(heading) for heading in details)
    assert '- [ ]' not in text
    assert 'No open question' in text or 'No A1 architecture question' in text

link_count = 0
for path in (importer_path, memory_path, core_path, verify_path, legalize_path):
    text = re.sub(r'```.*?```', '', path.read_text(), flags=re.S)
    text = re.sub(r'`[^`]*`', '', text)
    for target in re.findall(r'\[[^]]*\]\(([^)#]+)(?:#[^)]*)?\)', text):
        if '://' in target:
            continue
        assert (path.parent / target).resolve().exists(), (path, target)
        link_count += 1

handoff_rules = [
    (importer, 'one\nmove-only `ModuleDraft`'),
    (importer, '`verify_and_publish_raw(ModuleDraft&&)` and may mint one `RawBir`'),
    (importer, 'one move-only, fully verified, target-independent,\nunallocated `RawBir` to phase B'),
    (core, 'one complete move-only private `ModuleDraft`'),
    (core, 'The sole public phase-A output is one move-only immutable `RawBir`'),
    (core, 'B1 receives no draft, importer maps, hidden side table, malformed/unsupported valid row or target/allocation fact'),
    (verify, 'Only\n`verify_and_publish_raw(ModuleDraft&&)`'),
    (verify, 'Raw and Canonical BIR carry no semantic `target_profile`'),
    (legalize, 'The only input is an immutable, published `RawBir` view'),
    (legalize, 'B1 is never a producer-repair route'),
    (child_b, 'one accepted phase-A\n`RawBir` becomes one verified, unallocated `CanonicalBir`'),
]
for text, rule in handoff_rules:
    assert rule in text, rule
for text, status in ((importer, 'partial'), (memory, 'absent'), (core, 'partial')):
    assert f'Implementation-Status: {status}' in text
    assert 'unsupported valid' in text.lower(), status

assert 'Status: Open (deferred; not active)' in idea734
assert 'inactive until this idea is accepted' in child_a
assert 'This idea performs no implementation and does not\nactivate 734' in child_a
assert 'Predecessor: accepted `ideas/open/735_bir_phase_a_import_raw_document_convergence.md`' in child_b
assert 'Acceptance is strictly `A -> B -> C -> D -> E -> F`' in umbrella
assert 'Idea 734 remains a deferred inactive implementation consumer after\nChild-A acceptance' in umbrella

expected_commits = [
    '10d70b872b9a21d60524cf7688b26979622e472a',
    '060a32c7826d34d6f13396945231665e00bb5e2f',
    'e759322ab9c3fbfc1ced8e9475da1dec3f828844',
    '237afcdf62216d8ba19bb325e27c98c20bdd5982',
    'ba1dcab88cbe38d9beedcfcfe6a0dad9cb7fa5e3',
    '65a20c2d325d48589f4d114bae148172db244db4',
]
actual_commits = subprocess.check_output(
    ['git', 'rev-list', '--reverse', '934f6a985..HEAD'], text=True).splitlines()
assert actual_commits == expected_commits, actual_commits
expected_files = {
    expected_commits[0]: {'src/backend/bir/lir_to_bir/README.md', 'todo.md'},
    expected_commits[1]: {'src/backend/bir/lir_to_bir/memory/README.md', 'todo.md'},
    expected_commits[2]: {'src/backend/bir/core/README.md', 'todo.md'},
    expected_commits[3]: {'todo.md'},
    expected_commits[4]: {'plan.md', 'todo.md'},
    expected_commits[5]: {
        'src/backend/bir/LEGACY_COVERAGE.md', 'src/backend/bir/README.md',
        'src/backend/bir/compatibility/README.md',
        'src/backend/bir/passes/legalize/README.md',
        'src/backend/bir/verify/README.md', 'todo.md',
    },
}
for commit, expected in expected_files.items():
    changed = set(subprocess.check_output(
        ['git', 'diff-tree', '--no-commit-id', '--name-only', '-r', commit],
        text=True).splitlines())
    assert changed == expected, (commit, changed, expected)
for protected in ('src/codegen/lir',
                  'ideas/open/734_lir_to_new_bir_container_completeness.md'):
    changed = subprocess.check_output(
        ['git', 'diff', '--name-only', '934f6a985^..HEAD', '--', protected],
        text=True).strip()
    assert not changed, (protected, changed)

changed_now = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
assert changed_now == {'todo.md'}, changed_now
todo = Path('todo.md').read_text()
assert re.search(r'^Status: Active$', todo, re.M)
assert re.search(
    r'^Runbook State: Steps Exhausted — Awaiting Plan-Owner Judgment$',
    todo, re.M)
print('PASS inst=38 term=6 metadata=18 memory=11+11 twins=9 '
      f'links={link_count} handoff={len(handoff_rules)} history=6 '
      'protected=LIR+idea734 scope=todo lifecycle=active/exhausted '
      'next=plan-owner')
PY
git diff --check
```

- Result:

```text
PASS inst=38 term=6 metadata=18 memory=11+11 twins=9 links=31 handoff=11 history=6 protected=LIR+idea734 scope=todo lifecycle=active/exhausted next=plan-owner
git diff --check: PASS
```
