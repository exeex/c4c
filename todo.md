# Current Packet

Status: Active
Source Idea Path: ideas/open/811_lir_native_vector_authority_carrier_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish structured vector identities and facts

## Just Finished

Step 1 carrier audit completed. `LirInsertElementOp`,
`LirExtractElementOp`, and `LirShuffleVectorOp` in `ir.hpp` currently contain
only `LirOperand`/`LirTypeRef` presentation fields; `verify.cpp` admits each
by result/value operand kind and module type-reference well-formedness only.
The two scalar-splat sites in `expr/binary.cpp` construct insert then shuffle
through `fresh_tmp`, rendered vector/element/mask types, `"i64 0"`, `poison`,
and `zeroinitializer`. The vector `IndexExpr` path in `expr/misc.cpp` obtains
the vector and index through `emit_rval_id`, coerces the index to rendered
`i32`, then constructs extract with a `fresh_tmp` result.

The smallest shared boundary is one opt-in `LirNativeVectorAuthority` carrier
owned by these three vector-op schemas, not a parser or a row contract. It
contains: one current-function owner `LinkNameId`; required valid
current-function `LirValueId` result and each non-special value use; native
shape `{ lane_count > 0, element_type: LirTypeRef }` separately for every
vector input and result role (so shuffle input and result shapes do not imply
one another); an optional structured index `{ value: LirOperand,
type: LirTypeRef }` only on insert/extract; and ordered native shuffle lanes
`vector<LirShuffleMaskLane>`, where each lane is explicitly either inactive or
an unsigned selected-lane ordinal. Existing displayed operands/types remain
checked mirrors, never an authority source.

Carrier admission is fail-closed: when the carrier is opted in, reject a
missing/invalid or non-unique current-function owner; invalid, foreign,
undefined, or display-incoherent result/use IDs; absent/zero lane count or
invalid element/index type; absent index value/type or index display-mirror
mismatch; absent mask lanes, lane-count mismatch, or mask display-mirror
mismatch. Do not derive any fact from `%t`, rendered vector/index/mask text,
`poison`, or `zeroinitializer`. Special non-value tokens may remain display
mirrors and must not receive fabricated `LirValueId`s.

Positive carrier matrix: (1) scalar splat insert publishes a current-function
result plus scalar element use, native vector shape, and structured zero index;
(2) its shuffle publishes result/input IDs for the inserted vector, native
input/result shapes, and ordered all-zero mask lanes; (3) vector IndexExpr
extract publishes result/vector/index IDs, vector shape, and coerced `i32`
index fact. Malformed matrix: missing/foreign/undefined owner or IDs; any
display mirror inconsistent with a supplied ID/fact; zero/missing/mismatched
vector lanes or invalid element type; missing/index-type-incoherent index;
and missing/mismatched/malformed shuffle lanes. These checks establish carrier
coherence only: no selected-lane bounds, insert/extract element compatibility,
shuffle input compatibility, poison semantics, or 754 row admission is
selected or enabled.

## Suggested Next

Step 2 bounded implementation packet: add the reusable opt-in carrier adjacent
to the three vector schemas in `src/codegen/lir/ir.hpp`, add carrier-only
verification in `src/codegen/lir/verify.cpp`, and populate it only at the two
scalar-splat producer sites in `src/codegen/lir/hir_to_lir/expr/binary.cpp` and
the vector-`IndexExpr` extract site in `expr/misc.cpp`. Preserve checked
display mirrors and leave all other producers untouched. Do not add operation
semantics, tests/logs, or choose/enable a 754 row in this packet.

## Watchouts

No display-text recovery or synthetic IDs for `poison`/`zeroinitializer`.
Keep aggregate, CFG/PHI, pointers, Raw-BIR, target, MIR, emission, tests, and
all 754 row behavior outside this packet.

## Proof

Step 1 was read-only. Step 2 requires the executor's fresh build plus its
narrow carrier-only proof; Step 3 owns the positive/malformed test matrix and
must not prove row semantics.
