# Current Packet

Status: Active
Source Idea Path: ideas/open/735_bir_phase_a_import_raw_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit the shared A2 verifier and cross-cutting boundaries

## Just Finished

- Completed plan Step 3: converged `src/backend/bir/core/README.md` as the sole
  Raw receiving/storage/identity contract with the exact metadata spine,
  core-first sections and applicable detail-heading order.
- Added exhaustive receiving matrices in current source order for all 38
  `LirInst` alternatives, six terminators and 18 metadata families. Every row
  has exactly one typed target core owner or validation-only non-destination,
  one truthful checked-in disposition, and an ownership invariant. Old/new
  load/store/binary/cast/compare/call/GEP/select/inline-asm twins converge on
  the same semantic owner.
- Preserved substantive core architecture: disjoint module/function-owned
  stable IDs, generation-checked slot storage, explicit deterministic orders,
  one typed definition and exact def-use, terminator-only CFG successor slots,
  declaration/definition and initializer/object ownership, private revisions,
  transactional builders/editors and immutable views.
- Validation-only `module-context`, `intrinsic-requirements` and
  `producer-indexes-caches` rows publish no semantic duplicate.
  `target_profile`/rendered `data_layout`, ABI, constraints, target opcodes,
  homes, frames, allocation, MIR and emission remain outside Raw.
- Defined one private move-only `ModuleDraft`, exact-revision freeze, and the
  sole shared A2 `verify_and_publish_raw(ModuleDraft&&)` publication path with
  no builder/constructor/diagnostic bypass or partial publication. B1 receives
  only one move-only fully verified `RawBir`.
- Reconciled checked-in truth: function/block/instruction/value IDs, slot/order
  scaffolding, generic operand/results, small types, four terminator forms,
  basic functions/builders/views and only `Opcode::InlineAsm` exist. Missing
  stores/payloads/def-use/editors/revisions/private draft/full-gate path remain
  explicitly missing; unsupported valid rows and design prose are not coverage.
  No coordinated adjacent-owner blocker was found.

## Suggested Next

- Execute plan Step 4 as a read-only shared-boundary audit plus `todo.md`
  progress: compare the three phase-A owned documents with the shared A2
  verifier, analysis/invalidation, diagnostics, compatibility, coverage and
  review contracts. If any shared text must change, record the exact named
  coordinated-boundary seam and keep Step 4 active; do not edit shared owners.

## Watchouts

- Treat the core matrices as target ownership, not checked-in implementation.
  Preserve each row's explicit disposition during the shared audit.
- Confirm the shared verifier accepts the exact private draft revision and
  validation-only exclusions without duplicating core/importer authority.
- Shared analysis, diagnostics, compatibility, coverage and review documents
  are read-only in Step 4. Do not silently repair or reassign them.

## Proof

- Docs-only packet: no canonical regression log applies and neither
  `test_before.log` nor `test_after.log` was changed.
- Exact receiving-inventory, single-owner/disposition, twin convergence,
  heading, link, implementation-truth, checklist and scope proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

source = Path('src/codegen/lir/ir.hpp').read_text()
doc_path = Path('src/backend/bir/core/README.md')
doc = doc_path.read_text()

def source_variants(alias):
    match = re.search(rf'using {alias} = std::variant<(.*?)>;', source, re.S)
    assert match, alias
    return re.findall(r'\bLir[A-Za-z0-9_]+\b', match.group(1))

def receiving_rows(heading, end_heading, prefix):
    section = doc.split(heading, 1)[1].split(end_heading, 1)[0]
    result = []
    for line in section.splitlines():
        match = re.match(
            rf'^\| `{prefix}::(Lir[A-Za-z0-9_]+)` \|', line)
        if not match:
            continue
        cells = [cell.strip() for cell in line.split('|')[1:-1]]
        result.append((match.group(1), cells))
    return result

inst = receiving_rows(
    '## Exhaustive Instruction Receiving Matrix',
    '## Exhaustive Terminator Receiving Matrix', 'LirInst')
term = receiving_rows(
    '## Exhaustive Terminator Receiving Matrix',
    '## Exhaustive Metadata-Family Receiving Matrix', 'LirTerminator')
assert [name for name, _ in inst] == source_variants('LirInst')
assert [name for name, _ in term] == source_variants('LirTerminator')
assert len(inst) == 38 and len(term) == 6
for name, cells in inst + term:
    assert len(cells) == 4
    assert cells[1] and cells[2], (name, cells)
    assert any(word in cells[2] for word in
               ('missing', 'partial', 'checked-in')), (name, cells[2])

expected_metadata = [
    'module-context', 'stable-identities', 'operand-kinds', 'type-system',
    'functions-signatures', 'blocks-cfg-order', 'values-def-use',
    'stack-objects-allocas', 'globals-objects', 'initializers', 'strings',
    'externs', 'specializations', 'intrinsic-requirements',
    'inline-asm-metadata', 'producer-indexes-caches', 'source-order-origin',
    'module-publication',
]
metadata_section = doc.split(
    '## Exhaustive Metadata-Family Receiving Matrix', 1)[1].split(
    '## Ordered Behavior', 1)[0]
metadata = []
for line in metadata_section.splitlines():
    match = re.match(r'^\| `([a-z0-9-]+)` \|', line)
    if not match:
        continue
    cells = [cell.strip() for cell in line.split('|')[1:-1]]
    metadata.append((match.group(1), cells))
assert [name for name, _ in metadata] == expected_metadata
assert len(metadata) == 18
for name, cells in metadata:
    assert len(cells) == 4 and cells[1] and cells[2], (name, cells)
for name in ('module-context', 'intrinsic-requirements',
             'producer-indexes-caches'):
    cells = dict(metadata)[name]
    assert 'no semantic core owner' in cells[1], (name, cells[1])
    assert 'validation-only non-destination' in cells[2], (name, cells[2])

inst_owner = {name: cells[1] for name, cells in inst}
twins = [
    ('LirLoad', 'LirLoadOp', 'Load'),
    ('LirStore', 'LirStoreOp', 'Store'),
    ('LirBinary', 'LirBinOp', 'binary/unary'),
    ('LirCast', 'LirCastOp', 'Cast'),
    ('LirCmp', 'LirCmpOp', 'Compare'),
    ('LirCall', 'LirCallOp', 'Call'),
    ('LirGep', 'LirGepOp', 'GetElementPtr'),
    ('LirSelect', 'LirSelectOp', 'Select'),
    ('LirInlineAsm', 'LirInlineAsmOp', 'InlineAsm'),
]
for old, new, owner in twins:
    assert owner in inst_owner[old] and owner in inst_owner[new], (
        old, new, inst_owner[old], inst_owner[new])

spine = [
    'Contract-Status:', 'Implementation-Status:', 'Kind:', 'Phase-ID:',
    'Upstream:', 'Downstream:', 'Owner-Path:', 'Last-Reconciled-Commit:',
]
core = [
    '## Purpose', '## Owns', '## Does Not Own', '## Inputs', '## Outputs',
    '## Adjacent-Stage Contract',
]
detail = [
    '## Ordered Behavior', '## Invariants',
    '## Verification and Publication', '## Failure and Diagnostics',
    '## Analysis and Invalidation', '## Target and ABI Rules',
    '## Implementation State', '## Proof Requirements',
    '## Open Questions', '## Review Checklist',
]
for ordered in (spine, core, detail):
    positions = [doc.index(item) for item in ordered]
    assert positions == sorted(positions), (ordered, positions)

links = re.findall(r'\[[^]]+\]\(([^)#]+)(?:#[^)]*)?\)', doc)
assert links
for target in links:
    resolved = (doc_path.parent / target).resolve()
    assert resolved.exists(), (target, resolved)

required_truth = [
    'Implementation-Status: partial',
    'only `Opcode::InlineAsm`',
    '`target_profile` and rendered `data_layout` have no semantic core destination',
    'Only\n`verify_and_publish_raw(ModuleDraft&&)`',
    'cannot bypass this boundary',
    'foundation adapter',
    'unsupported valid rows and target design prose are not coverage',
]
for statement in required_truth:
    assert statement in doc, statement
checklist = doc.split('## Review Checklist', 1)[1]
assert '- [ ]' not in checklist
assert checklist.count('- [x]') == 10

changed = {
    line for line in subprocess.check_output(
        ['git', 'diff', '--name-only'], text=True).splitlines() if line
}
assert changed == {'src/backend/bir/core/README.md', 'todo.md'}, changed
print('PASS LirInst=38 LirTerminator=6 metadata=18 owners=62 '
      f'twins={len(twins)} spine=8 core=6 detail=10 links={len(links)} '
      'checklist=10/10 changed=2')
PY
git diff --check
```

- Result:

```text
PASS LirInst=38 LirTerminator=6 metadata=18 owners=62 twins=9 spine=8 core=6 detail=10 links=13 checklist=10/10 changed=2
git diff --check: PASS
```
