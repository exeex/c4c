# P02 Scalar Canonicalization Pass Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`P02` is the `B2` transformation. It converts the exact committed B1 / P01
output into normalized target-independent scalar, comparison, cast, and select
forms for the B3 / P03 successor.

## 1. Exact input and analysis checkpoint

The input is one immutable BIR view with exact epoch, module revision, observed
function revisions, and `PassProperty::TypesLegal`. The P01 postcondition and
configured input verifier must accept that same revision. Raw scalar aliases,
unowned special operations, or stale/mismatched capability stamps reject the
invocation before mutation.

P02 may request `ComparisonSelect` analysis only through the analysis manager.
The handle must match the exact input revision and schema/options key. It is an
immutable planning aid, not authority to edit BIR, and a stale handle is an
explicit failure rather than a request to retarget it.

## 2. Closed authority

P02 owns exactly:

- canonical commutative operand order by stable semantic order;
- canonical comparison predicate and operand orientation while retaining
  signedness and floating ordered/unordered behavior;
- target-independent constant folding and identity/algebraic rewrites whose
  poison, undef, overflow, trapping, rounding, and exceptional behavior is
  proven identical by a closed rule;
- redundant scalar cast removal and canonical cast-chain composition under the
  same semantic proof requirement;
- canonical `Select` condition/result shape, constant-arm folding, and bounded
  select-chain normalization;
- width-parametric expansion of a `WideIntegerOp` or other registered special
  semantic operation only when a closed portable rule represents the exact
  semantics using canonical BIR;
- preserving an unexpanded special semantic node only when its registry entry
  names an exact later semantic owner.

P02 does not change storage types or layouts, call signatures, memory access
semantics, aggregate topology, CFG successors, phi placement, symbol identity,
or intrinsic ownership. It cannot introduce helper symbols, machine-facing
instruction sequences, calling locations, register identities, stack state,
allocation facts, or encodings.

Inline-assembly template text, constraint text, clobber spellings, side-effect
flags, ordinary inputs/results, and asm-goto topology are preserved exactly.
P02 neither interprets nor rewrites opaque payload.

## 3. Canonical scalar rules

The closed rule registry is keyed by stable `ScalarRuleId`; free-form peephole
names cannot authorize a rewrite. Each rule declares operand/result domains,
semantic preconditions, allowed mutation effects, origin policy, and required
proof for poison, undef, exceptional, and floating behavior.

Canonical results obey these invariants:

- boolean conditions are `i1`; boolean folding never observes a concrete value
  for `Undef` or `Poison`;
- integer operations retain exact width and explicit signedness where semantics
  differ; shifts retain their declared count policy;
- floating operations retain exact format, rounding/exception behavior, NaN
  class and payload requirements, and signed-zero distinctions;
- comparisons use the closed P01 predicate set and one deterministic
  orientation; pointer relations are never obtained through an integer cast;
- `Select` has one `i1` condition and equal typed arms/result; a fold occurs
  only when it preserves lazy/poison/undef semantics defined by the core;
- a special operation is either expanded completely by one registered portable
  rule or retained with one named later owner. Partial expansion is forbidden.

P02 uses an internally bounded deterministic worklist and must be idempotent
after one successful invocation. The built-in pipeline does not run an outer
fixed-point loop. Local rewrites cannot invoke or reorder another pass. Budgets
are deterministic work/entity/diagnostic limits supplied by the pass framework.

## 4. Transaction and exact output

`PassId::ScalarCanonicalize` is a `Function` pass requiring `TypesLegal`,
establishing `ScalarsCanonical`, and using `RepeatContract::Idempotent`. Each
function transaction inventories all candidate rewrites in canonical entity
order, rejects the complete candidate if any operation lacks a valid
disposition, applies deterministic replacements and typed RAUW, derives the
authoritative `MutationSummary`, and runs P02 postconditions plus
verifier-on-commit. The pipeline publishes the complete function wave once;
failure in any function discards the unpublished occurrence candidate.

Success publishes exactly one immutable B2 revision:

- the input epoch and framework-derived module/function revisions;
- `PassProperty::ScalarsCanonical` established by the executor;
- canonical scalar opcode, comparison, cast, and select forms;
- a complete registered disposition for every retained wide, extended-float,
  complex, checked, or otherwise special semantic operation;
- no target-dependent or allocation-domain facts;
- unchanged opaque inline-assembly payload and transport.

All result identity changes use deterministic new stable IDs, typed atomic
RAUW, complete def-use repair, and composed origins. An unchanged invocation
keeps the revision and must prove the repeat contract's no-op condition.

## 5. Reject and failure behavior

P02 rejects unsupported semantics explicitly. `UnsupportedCanonicalSemantics`
means no exact portable canonical form exists; `MissingDownstreamDisposition`
means retention was requested without a registered later owner. Other closed
failures include stale input or analysis, rule-precondition mismatch,
nonconvergence, semantic-proof failure, forbidden authority, deterministic
resource exhaustion, cancellation, verifier rejection, and transaction
failure.

Every failure rolls back the entire candidate and publishes no output property,
revision, partial fold, cache result, or stage token. P02 cannot replace failure
with a generic unsupported node, silently weaken exceptional behavior, or keep
an unregistered operation for a later phase.
