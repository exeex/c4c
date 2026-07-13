# Current Packet

Status: Active
Source Idea Path: ideas/open/735_bir_phase_a_import_raw_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Converge the A1 memory import sub-boundary

## Just Finished

- Completed plan Step 1: substantively converged
  `src/backend/bir/lir_to_bir/README.md` as the sole top-level A1 importer
  owner with the exact metadata spine and core-first ownership/input/output/
  adjacency order, followed by the umbrella's exact applicable detail-heading
  order. `Last-Reconciled-Commit` is truthfully `none` until this document
  change is committed.
- The importer document now contains exactly 38 individually named current
  `LirInst` rows, six individually named current `LirTerminator` rows, and all
  18 exact metadata-family rows in source/contract order. Every row records
  authority class, one typed Raw destination or validation-only
  non-destination, importer rule, current disposition, full A2 verifier rule,
  stable failure, and positive plus malformed/neighbor proof obligation.
- Reconciled implementation truth to the build-included top-level importer and
  checked-in core: bounded `LirInlineAsmOp`, branch/void-return/unreachable,
  function/block and small-type coverage remains partial; build-excluded nested
  migration sources and design tables are not implementation evidence.
- Current LIR remains complete and immutable. The existing inline-asm ordinary
  value/type/role/constraint-index/original-text carrier is sufficient; the
  missing Raw role/index owner and importer wiring are the receiving gap.
  `target_profile` and rendered `data_layout` are validation/origin/parity-only
  non-destinations; C1 selects its own profile and C2 derives layout.
- Defined one failure-atomic private import transaction, the sole
  `ModuleDraft -> verify_and_publish_raw(ModuleDraft&&) -> RawBir` A2 boundary,
  and the exact B1 handoff to the legalize contract. Existing shared A2 and B1
  clauses accept this adjacency, so no coordinated shared-owner blocker was
  found in Step 1.

## Suggested Next

- Execute plan Step 2 on
  `src/backend/bir/lir_to_bir/memory/README.md` only: make it a truthful
  build-excluded subordinate migration boundary, bind every memory row to the
  top-level dispatcher and one Raw owner, and forbid independent publication,
  scalarization, target layout, ABI or machine-lowering authority.

## Watchouts

- Preserve the top-level importer as the only 38/6 dispatcher and A1
  transaction owner. The memory document may refine validation but cannot
  duplicate dispatch, storage, verifier, or publication authority.
- Keep build-excluded implementation status explicit. A stable unsupported
  diagnostic remains failure evidence, not receipt of a valid current row.
- Do not edit core/shared verifier/B1 text during Step 2. Record an exact
  coordinated-boundary seam if the memory sub-boundary cannot agree without
  such an edit.

## Proof

- Docs-only packet: no canonical regression log applies and neither
  `test_before.log` nor `test_after.log` was changed.
- Exact structural, inventory, link, ownership and diff proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re

source_path = Path('src/codegen/lir/ir.hpp')
doc_path = Path('src/backend/bir/lir_to_bir/README.md')
source = source_path.read_text()
doc = doc_path.read_text()

def source_variants(alias):
    match = re.search(rf'using {alias} = std::variant<(.*?)>;', source, re.S)
    assert match, alias
    return re.findall(r'\bLir[A-Za-z0-9_]+\b', match.group(1))

def recorded_rows(heading, prefix, end_heading):
    section = doc.split(heading, 1)[1].split(end_heading, 1)[0]
    return re.findall(
        rf'^\| `{prefix}::(Lir[A-Za-z0-9_]+)` \|', section, re.M)

inst = recorded_rows(
    '## Exhaustive Instruction Input Matrix', 'LirInst',
    '## Exhaustive Terminator Input Matrix')
term = recorded_rows(
    '## Exhaustive Terminator Input Matrix', 'LirTerminator',
    '## Exhaustive Metadata-Family Input Matrix')
assert len(inst) == len(set(inst)) == 38
assert len(term) == len(set(term)) == 6
assert inst == source_variants('LirInst')
assert term == source_variants('LirTerminator')

expected_families = [
    'module-context', 'stable-identities', 'operand-kinds', 'type-system',
    'functions-signatures', 'blocks-cfg-order', 'values-def-use',
    'stack-objects-allocas', 'globals-objects', 'initializers', 'strings',
    'externs', 'specializations', 'intrinsic-requirements',
    'inline-asm-metadata', 'producer-indexes-caches', 'source-order-origin',
    'module-publication',
]
metadata_section = doc.split(
    '## Exhaustive Metadata-Family Input Matrix', 1)[1].split(
    '## Ordered Behavior', 1)[0]
metadata = re.findall(r'^\| `([a-z0-9-]+)` \|', metadata_section, re.M)
assert metadata == expected_families
assert len(metadata) == len(set(metadata)) == 18

spine = [
    'Contract-Status:', 'Implementation-Status:', 'Kind:', 'Phase-ID:',
    'Upstream:', 'Downstream:', 'Owner-Path:', 'Last-Reconciled-Commit:',
]
core_headings = [
    '## Purpose', '## Owns', '## Does Not Own', '## Inputs', '## Outputs',
    '## Adjacent-Stage Contract',
]
detail_headings = [
    '## Ordered Behavior', '## Invariants',
    '## Verification and Publication', '## Failure and Diagnostics',
    '## Analysis and Invalidation', '## Target and ABI Rules',
    '## Implementation State', '## Proof Requirements',
    '## Open Questions', '## Review Checklist',
]
for ordered in (spine, core_headings, detail_headings):
    positions = [doc.index(item) for item in ordered]
    assert positions == sorted(positions), (ordered, positions)

links = re.findall(r'\[[^]]+\]\(([^)#]+)(?:#[^)]*)?\)', doc)
assert links
for target in links:
    resolved = (doc_path.parent / target).resolve()
    assert resolved.exists(), (target, resolved)

assert 'Implementation-Status: partial' in doc
assert 'Last-Reconciled-Commit: none' in doc
assert 'verify_and_publish_raw(ModuleDraft&&)' in doc
assert 'validation/origin/parity-only non-destination' in doc
assert 'No LIR edit or exception is justified' in doc
checklist = doc.split('## Review Checklist', 1)[1]
assert '- [ ]' not in checklist
assert checklist.count('- [x]') == 10

changed = {
    line for line in __import__('subprocess').check_output(
        ['git', 'diff', '--name-only'], text=True).splitlines() if line
}
assert changed == {
    'src/backend/bir/lir_to_bir/README.md', 'todo.md'
}, changed
print('PASS LirInst=38 LirTerminator=6 metadata=18 spine=8 '
      'core_headings=6 detail_headings=10 checklist=10/10 '
      'reconciled=none links=%d changed=2' % len(links))
PY
git diff --check
```

- Result:

```text
PASS LirInst=38 LirTerminator=6 metadata=18 spine=8 core_headings=6 detail_headings=10 checklist=10/10 reconciled=none links=9 changed=2
git diff --check: PASS
```
