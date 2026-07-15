# Current Packet

Status: Active
Source Idea Path: ideas/open/746_bir_node_kind_centric_storage_pass_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit the current BIR core contract and query duplication

## Just Finished

- Completed `plan.md` Step 1 as a read-only architecture audit. The smallest
  insertion seam is in `src/backend/bir/core/ir.hpp`, immediately after the
  current `enum class Opcode` and before payload declarations: make that closed
  vocabulary the `NodeKind` authority (initially by a compatibility-preserving
  rename/alias), and keep the descriptor/traits implementation in one adjacent
  core schema layer rather than beside individual passes or payload types.
- The initial schema family should expose hidden `node_kind_traits<K>` plus
  pass-facing compile-time queries and runtime wrappers for: semantic family
  (`is_op_semantic_v<K>` / `is_op_semantic(kind)`), binary classification,
  fixed/variable input arity and operand roles, zero/one/many result policy,
  memory/effect policy, payload alternatives, and legal stage mask. Runtime
  lookup must use an exhaustive `switch`/closed descriptor table and fail
  closed for invalid values; concrete `Type` remains a value fact rather than
  a kind-only answer.
- The first real consumer seam is
  `src/backend/bir/verify/verifier.cpp::opcode_matches_payload`, which currently
  duplicates the complete opcode-to-payload relation. Nearby duplication also
  exists in builder/verifier producer classification through repeated
  `std::holds_alternative<LoadNode>`, `CallNode`, `AbsNode`,
  `IntrinsicCallNode`, and `SelectNode` probes. Step 2 should centralize the
  former; the latter are bounded candidates for the later one-consumer
  migration, not grounds for a whole verifier/builder rewrite.
- No BIR `.td` or TableGen file exists today; the replacement target is the
  emerging need for one shared classification/behavior authority, not removal
  of a checked-in generator.
- Bootstrap compatibility fields are `detail::InstData::operands` and
  `results` as heap-owning `std::vector<ValueId>`, `ValueDef::type`, and
  `InstResultDef::{instruction,result_index}`. They preserve the current
  separate value model and explicit multi-result indexing. Step 2 must not
  remove them: declare zero/one/many result policy in the schema while retaining
  vectors and `result_index`; choosing single-result normalization versus a
  compact `ResultSpan`/external result table remains an explicit bounded
  follow-up. `InstId` already supplies slot-map identity and is not stored in
  `InstData`.

## Suggested Next

- Execute a bounded `plan.md` Step 2 packet in core only: introduce the
  `NodeKind` compatibility name and one constexpr traits/descriptor authority
  for the existing 16-kind vocabulary; expose compile-time/runtime queries for
  family, binary-ness, operand/result arity, payload acceptance, effects, and
  stage legality; replace only `opcode_matches_payload` with the shared payload
  query; add positive/negative schema tests across semantic, memory, call, and
  authority kinds. Preserve `InstData`, value/result vectors, and builders.

## Watchouts

- `Opcode::Store`, `Load`, `GetElementPtr`, and `Call` each accept multiple
  payload alternatives, so payload policy cannot be modeled as a single
  enum-to-variant-index equality. Keep the accepted-alternative relation in the
  one schema and allow payload-aware effect refinement (notably inline asm and
  calls) without moving semantic authority into payloads.
- The README and closed 735 contract still describe ordered generic result
  vectors, while idea 746 prefers node/value identity and explicit
  normalization for multi-result forms. Treat this as named compatibility
  scaffolding, not permission to change storage during Step 2.
- The required clang-tool install could not run because
  `ref/c4c-clang-tools` lacks `CMakeLists.txt`, and no installed or build-local
  binaries were available. The audit therefore used bounded textual inspection
  after attempting the mandated AST-tool route; implementation should repair
  the tool checkout before relying on AST-backed caller/reference proof.

## Proof

- Supervisor-selected proof: `git diff --check > test_after.log 2>&1`.
- Result: passed; log path `test_after.log`.
- This is sufficient for the read-only design packet; no build or test subset
  was appropriate because no C++ or documentation implementation file changed.
