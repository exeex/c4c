# MLIR vs C++: mxfp4 MatMul with DMA Software Pipeline

This document walks through a concrete example: implementing a tiled mxfp4 MatMul
with DMA software pipelining, comparing the MLIR path against the c4c C++ path.

The example is chosen deliberately. mxfp4 is a type that MLIR's standard dialects
do not know about. DMA software pipelines are a timing-sensitive pattern that
MLIR's lowering pipeline cannot represent without losing the intent.
Both problems appear together in every real NPU matmul kernel.

---

## The Target

We want to compute a matrix multiplication where the weight matrix is stored as
tiled mxfp4. The memory layout is:

```
Tensor<Tile<fp4x2, 32, 32>, 2, 64>
```

This means:

```
outer grid:   [2, 64]            — number of tiles per dimension
tile shape:   [32, 32]           — elements per tile
element type: fp4x2              — two fp4 values packed into one byte
total size:   2 × 64 × 32 × 32 × 0.5 byte = 2 × 64 × 512 bytes
```

DMA moves one tile at a time — 512 bytes — directly into SRAM for the tensor core.
The pipeline overlaps DMA of tile N+1 with tensor core computation of tile N.

---

## Part 1: Defining the Type

### MLIR path

Before writing a single line of kernel logic, you must build the entire
compiler infrastructure that lets MLIR understand your type.
This is not optional. Every piece below is required.

#### Step 1: TableGen dialect definition

```tablegen
// npu_dialect.td
include "mlir/IR/DialectBase.td"
include "mlir/IR/AttrTypeBase.td"

def NPU_Dialect : Dialect {
  let name = "npu";
  let summary = "NPU dialect for mxfp4 operations";
  let cppNamespace = "::mlir::npu";
  let useDefaultTypePrinterParser = 0;  // we need custom parsing
}

def NPU_FP4Type : TypeDef<NPU_Dialect, "FP4", []> {
  let mnemonic = "fp4";
  let summary = "4-bit floating point";
  let assemblyFormat = "";
}

def NPU_FP4x2Type : TypeDef<NPU_Dialect, "FP4x2", []> {
  let mnemonic = "fp4x2";
  let summary = "packed pair of fp4 values";
  let assemblyFormat = "";
}

// Tile type: fixed 2D block of elements
def NPU_TileType : TypeDef<NPU_Dialect, "Tile", []> {
  let mnemonic = "tile";
  let summary = "2D tile with fixed shape";
  let parameters = (ins
    "mlir::Type":$elementType,
    "int64_t":$rows,
    "int64_t":$cols
  );
  let assemblyFormat = "`<` $elementType `,` $rows `x` $cols `>`";
  let builders = [
    TypeBuilder<(ins "mlir::Type":$elementType,
                     "int64_t":$rows,
                     "int64_t":$cols)>
  ];
}
```

#### Step 2: C++ type storage class

Every parametric MLIR type needs a TypeStorage class:

```cpp
// NPUTypeStorage.h
namespace mlir::npu::detail {

struct TileTypeStorage : public mlir::TypeStorage {
  using KeyTy = std::tuple<mlir::Type, int64_t, int64_t>;

  TileTypeStorage(mlir::Type elementType, int64_t rows, int64_t cols)
      : elementType(elementType), rows(rows), cols(cols) {}

  bool operator==(const KeyTy &key) const {
    return key == KeyTy{elementType, rows, cols};
  }

  static llvm::hash_code hashKey(const KeyTy &key) {
    return llvm::hash_combine(std::get<0>(key),
                               std::get<1>(key),
                               std::get<2>(key));
  }

  static TileTypeStorage *construct(mlir::TypeStorageAllocator &allocator,
                                    const KeyTy &key) {
    return new (allocator.allocate<TileTypeStorage>())
        TileTypeStorage(std::get<0>(key), std::get<1>(key), std::get<2>(key));
  }

  mlir::Type elementType;
  int64_t rows;
  int64_t cols;
};

} // namespace mlir::npu::detail
```

#### Step 3: C++ type class

```cpp
// NPUTypes.h
namespace mlir::npu {

class FP4Type : public mlir::Type::TypeBase<FP4Type, mlir::Type,
                                             mlir::TypeStorage> {
public:
  using Base::Base;
  static FP4Type get(mlir::MLIRContext *ctx);
  static bool classof(mlir::Type type);
  static constexpr mlir::StringLiteral name = "npu.fp4";
};

class FP4x2Type : public mlir::Type::TypeBase<FP4x2Type, mlir::Type,
                                               mlir::TypeStorage> {
public:
  using Base::Base;
  static FP4x2Type get(mlir::MLIRContext *ctx);
  static bool classof(mlir::Type type);
  static constexpr mlir::StringLiteral name = "npu.fp4x2";
};

class TileType : public mlir::Type::TypeBase<TileType, mlir::Type,
                                              detail::TileTypeStorage> {
public:
  using Base::Base;
  static TileType get(mlir::MLIRContext *ctx, mlir::Type elementType,
                      int64_t rows, int64_t cols);
  static bool classof(mlir::Type type);
  static constexpr mlir::StringLiteral name = "npu.tile";

  mlir::Type getElementType() const;
  int64_t getRows() const;
  int64_t getCols() const;
  int64_t getNumElements() const { return getRows() * getCols(); }
  int64_t getSizeInBytes() const;  // must account for fp4 packing
};

} // namespace mlir::npu
```

#### Step 4: C++ type class implementation

```cpp
// NPUTypes.cpp
namespace mlir::npu {

FP4Type FP4Type::get(mlir::MLIRContext *ctx) {
  return Base::get(ctx);
}

FP4x2Type FP4x2Type::get(mlir::MLIRContext *ctx) {
  return Base::get(ctx);
}

TileType TileType::get(mlir::MLIRContext *ctx, mlir::Type elementType,
                        int64_t rows, int64_t cols) {
  return Base::get(ctx, elementType, rows, cols);
}

mlir::Type TileType::getElementType() const {
  return getImpl()->elementType;
}

int64_t TileType::getRows() const { return getImpl()->rows; }
int64_t TileType::getCols() const { return getImpl()->cols; }

int64_t TileType::getSizeInBytes() const {
  int64_t elems = getNumElements();
  // fp4x2: two elements per byte; fp4: round up
  if (getElementType().isa<FP4x2Type>()) return elems / 2;
  if (getElementType().isa<FP4Type>())   return (elems + 1) / 2;
  // fallback: assume element has a known byte width
  auto intType = getElementType().dyn_cast<mlir::IntegerType>();
  return elems * (intType.getWidth() / 8);
}

} // namespace mlir::npu
```

#### Step 5: Dialect registration and type parser/printer

```cpp
// NPUDialect.cpp
#include "NPUDialect.h"
#include "NPUTypes.h"

void mlir::npu::NPUDialect::initialize() {
  addTypes<FP4Type, FP4x2Type, TileType>();
}

mlir::Type mlir::npu::NPUDialect::parseType(mlir::DialectAsmParser &parser) const {
  llvm::StringRef keyword;
  if (parser.parseKeyword(&keyword)) return {};

  if (keyword == "fp4")   return FP4Type::get(parser.getContext());
  if (keyword == "fp4x2") return FP4x2Type::get(parser.getContext());

  if (keyword == "tile") {
    if (parser.parseLess()) return {};
    mlir::Type elementType;
    if (parser.parseType(elementType)) return {};
    if (parser.parseComma()) return {};
    int64_t rows, cols;
    if (parser.parseInteger(rows)) return {};
    if (parser.parseKeyword("x")) return {};
    if (parser.parseInteger(cols)) return {};
    if (parser.parseGreater()) return {};
    return TileType::get(parser.getContext(), elementType, rows, cols);
  }

  parser.emitError(parser.getNameLoc(), "unknown NPU type: ") << keyword;
  return {};
}

void mlir::npu::NPUDialect::printType(mlir::Type type,
                                       mlir::DialectAsmPrinter &printer) const {
  if (type.isa<FP4Type>())   { printer << "fp4";   return; }
  if (type.isa<FP4x2Type>()) { printer << "fp4x2"; return; }

  if (auto t = type.dyn_cast<TileType>()) {
    printer << "tile<";
    printer.printType(t.getElementType());
    printer << ", " << t.getRows() << "x" << t.getCols() << ">";
    return;
  }
}
```

#### Step 6: Schedule model in TableGen

To teach MLIR's backend how to handle the new ops, you need a scheduling model:

```tablegen
// NPUScheduleModel.td
def NPU_ScheduleModel : SchedMachineModel {
  let CompleteModel = 1;
  let NumMicroOpBufferSize = 0;
}

// DMA engine resource
def NPU_DMAEngine : ProcResource<1>;  // one DMA channel

// Tensor core resource
def NPU_TensorCore : ProcResource<1>;

// DMA issue: occupies DMA engine for 1 cycle, latency = L (hardware specific)
def : WriteRes<WriteNPUDMAIssue, [NPU_DMAEngine]> {
  let Latency = 64;  // cycles until data is available — must be measured on FPGA
  let ResourceCycles = [1];
}

// Tensor core matmul: occupies tensor core, latency depends on tile size
def : WriteRes<WriteNPUMatMul, [NPU_TensorCore]> {
  let Latency = 32;
  let ResourceCycles = [32];
}

// DMA wait: no resource, but creates a hazard if DMA not done
def : WriteRes<WriteNPUDMAWait, []> {
  let Latency = 1;
}

// Read dependency: matmul cannot start until DMA data is ready
def : ReadAdvance<ReadNPUTileData, -64>;  // negative = must wait
```

This schedule model must then be wired into every op definition in TableGen,
referenced in the backend target description, and validated against actual
hardware timing. If the latency numbers are wrong, the scheduler will generate
incorrect overlap decisions — silently.

#### Step 7: Lowering pass for TileType to memref

Because standard passes do not understand `npu.tile`, you need a conversion pass
that lowers it to strided memref before bufferization:

```cpp
// LowerTileType.cpp
struct LowerTileTypePass
    : public mlir::PassWrapper<LowerTileTypePass, mlir::OperationPass<>> {

  void runOnOperation() override {
    mlir::ConversionTarget target(getContext());
    target.addIllegalDialect<npu::NPUDialect>();
    target.addLegalDialect<mlir::memref::MemRefDialect>();

    mlir::RewritePatternSet patterns(&getContext());
    patterns.add<TileTypeToMemRefPattern>(&getContext());

    if (mlir::failed(mlir::applyFullConversion(
            getOperation(), target, std::move(patterns))))
      signalPassFailure();
  }
};

struct TileTypeToMemRefPattern
    : public mlir::OpConversionPattern<npu::TileOp> {

  mlir::LogicalResult matchAndRewrite(
      npu::TileOp op,
      npu::TileOp::Adaptor adaptor,
      mlir::ConversionPatternRewriter &rewriter) const override {

    auto tileType = op.getType().cast<npu::TileType>();
    // Flatten tile to 1D i8 memref — tile structure is lost here
    auto flatSize = tileType.getSizeInBytes();
    auto memrefType = mlir::MemRefType::get({flatSize},
                          rewriter.getIntegerType(8));
    rewriter.replaceOpWithNewOp<mlir::memref::AllocOp>(op, memrefType);
    return mlir::success();
  }
};
```

Note that `getSizeInBytes()` must be correct for fp4x2 packing.
And after this pass runs, the tile structure is gone — replaced by a flat `i8`
memref. Every downstream pass that needs to know the tile dimensions
must re-derive them from the size, which may or may not be possible.

#### What you have written so far — before any kernel logic

```
npu_dialect.td          ~  50 lines  TableGen dialect + type defs
NPUTypeStorage.h        ~  40 lines  storage class for TileType
NPUTypes.h              ~  50 lines  type class declarations
NPUTypes.cpp            ~  40 lines  type class implementations
NPUDialect.cpp          ~  60 lines  registration, parser, printer
NPUScheduleModel.td     ~  35 lines  scheduling model
LowerTileType.cpp       ~  60 lines  lowering pass
CMakeLists.txt changes  ~  20 lines  build system wiring
─────────────────────────────────────
Total                   ~ 355 lines  zero kernel logic written yet
```

And this does not include:
- The `npu.matmul_fp4x2` op definition (another ~40 lines of TableGen + verifier)
- The op lowering pass to actual instructions (~60 lines)
- The DMA op definitions and their lowering
- Integration tests for the type system
- Any handling of the type in existing generic passes that touch memrefs

The tiled layout is still not first-class. After `LowerTileTypePass`,
the weight matrix is a flat `i8` memref with strides encoding the tile geometry:

```mlir
// What the IR looks like after lowering TileType
%weight = memref<65536xi8>  // 2 × 64 × 512 bytes, flat
// The fact that this is 128 tiles of 512 bytes each
// now lives only in the stride arithmetic of the access pattern.
// No pass will recover it.
```

### C++ path

```cpp
// fp4x2: two fp4 values packed into one byte
struct fp4x2 {
    uint8_t data;  // low nibble = first, high nibble = second

    static constexpr int bits = 4;
    static constexpr int pack = 2;
};

// Tile<T, Rows, Cols>: a contiguous block of Rows×Cols elements of type T
template<typename T, int Rows, int Cols>
struct Tile {
    static constexpr int rows = Rows;
    static constexpr int cols = Cols;
    static constexpr int size = Rows * Cols;
    static constexpr size_t bytes = size * sizeof(T);

    T data[size];
} __attribute__((packed, aligned(64)));

// Tensor<T, Dims...>: a multi-dimensional array of T
template<typename T, int... Dims>
struct Tensor {
    static constexpr size_t tile_bytes = sizeof(T);
    T tiles[(Dims * ...)];

    T& at(auto... idx);          // multi-index access
    T* data() { return tiles; }
};

// The weight matrix: 2×64 grid of 32×32 fp4x2 tiles
using WeightTile = Tile<fp4x2, 32, 32>;
using WeightMatrix = Tensor<WeightTile, 2, 64>;
```

`sizeof(WeightTile)` is exactly 512 bytes. The tile boundary is a first-class
concept in the type, not an implicit convention encoded in strides.
Every function that receives a `WeightTile*` knows exactly what it is handling.

The opcode for loading this type:

```cpp
template<typename T>
consteval const char* vload_opcode();

template<>
consteval const char* vload_opcode<WeightTile>() {
    return "dma.tile.load.fp4x2";
}
```

Adding a new tile type means adding one specialization. Nothing else changes.

---

## Part 2: The MatMul Kernel

### MLIR path

```mlir
// linalg.matmul operates on memrefs, not tiles
// The tiled structure must be manually expressed as loop nests

func @matmul_mxfp4(%A: memref<64x32xf32>,
                   %B: memref<2x64x32x16xi8>,  // weight, tiled layout in strides
                   %C: memref<64x64xf32>) {

  // Outer loops over tile grid
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 64 {

      // Inner loops over tile elements
      // fp4x2 unpacking is not expressible in standard linalg
      // You need a custom op or an explicit unpack sequence
      %tile = memref.subview %B[%i, %j, 0, 0][1, 1, 32, 16][1, 1, 1, 1]
                : memref<2x64x32x16xi8> to memref<32x16xi8>

      // At this point you need a custom op to unpack fp4x2 and compute
      // No standard dialect can express this
      npu.matmul_fp4x2 %A, %tile, %C : ...
    }
  }
}
```

The `npu.matmul_fp4x2` op requires its own dialect definition, verifier,
lowering pass, and pattern rewriter. The tile loop structure is now expressed
as affine loops — which is fine — but the semantic connection between the
loop bounds and the tile size exists only in the programmer's head.

### C++ path

```cpp
template<typename T>
consteval const char* matmul_opcode();

template<>
consteval const char* matmul_opcode<WeightTile>() {
    return "npu.matmul.fp4x2";
}

// Kernel: compute one output tile using one weight tile
template<typename WeightT>
inline void matmul_tile(float* out, float* act, WeightT* weight) {
    // Load activation
    asm volatile("npu.load.f32 act, %0" : : "r"(act));
    // Compute — opcode derived from WeightT at compile time
    asm volatile(matmul_opcode<WeightT>() " out, act, %0" : : "r"(weight));
    // Store result
    asm volatile("npu.store.f32 %0, out" : : "r"(out));
}
```

The tile type drives the opcode. The loop structure is expressed in C++,
not encoded implicitly in affine loop bounds. The tile boundary is explicit
because `WeightT*` is a `Tile<fp4x2, 32, 32>*` — the size is in the type.

---

## Part 3: DMA Software Pipeline

This is where the semantic gap between MLIR and C++ becomes most visible.

### MLIR path

#### There is no DMA in MLIR

MLIR has no DMA primitive. The closest approximation is `memref.copy` combined
with the `async` dialect:

```mlir
%token = async.execute {
  memref.copy %src_tile, %sram_buf : memref<512xi8> to memref<512xi8>
  async.yield
}
npu.matmul_fp4x2 %act, %sram_prev, %out
async.await %token : !async.token
```

`memref.copy` is not a DMA instruction. It is a memory transfer abstraction.
The lowering pass decides how to implement it — scalar loads, DMA, or something
else entirely. You do not control this, and the choice is not visible in the IR.

#### DMA lifecycle is not representable

A real DMA operation has a lifecycle:

```
cycle K:     issue descriptor        — DMA engine starts
cycle K+1:   engine reads src        — src must not change
...
cycle K+L:   engine writes dst done  — dst is now valid
cycle K+L+1: compute can read dst    — earliest safe read
```

`async.token` has two states: satisfied and unsatisfied.
It captures the dependency edge `compute must happen after copy`,
but it carries none of the following:

- Which cycle the descriptor was issued
- How long the transfer takes (latency model)
- Whether the src memory is locked during transfer
- Whether the DMA channel is occupied and blocks further issues
- What hardware mechanism implements the wait (poll bit, fence, interrupt)

All of these are decided by the lowering pass, invisibly, without guidance
from the IR. If the pass gets any of them wrong, the result is a silent
correctness or performance bug.

#### Loop-carried pipeline is structurally inexpressible

The natural software pipeline pattern is:

```
iteration i:
  issue DMA(i+1)    ← prefetch next input while computing current
  await DMA(i)      ← wait for this iteration's data
  compute(i)
```

In MLIR this requires loop-carried `async.token` values via `scf.for iter_args`:

```mlir
%tok_init = async.execute { memref.copy %dram[0], %sram_a }

scf.for %i = 1 to %N step 1
    iter_args(%read_tok = %tok_init, %active_buf = %sram_a) {

  // Issue next
  %next_tok = async.execute {
    memref.copy %dram[%i], %sram_b
    async.yield
  }

  // Await current — but which buffer is "current"?
  async.await %read_tok

  // Compute
  npu.matmul_fp4x2 %act, %active_buf, %out

  // Buffer swap: how do we pass the swapped buffer as iter_arg?
  // %sram_a and %sram_b are fixed SSA values — they cannot be swapped
  scf.yield %next_tok, ???
}
```

The buffer rotation — the core of double buffering — has no representation
in `scf.for iter_args`. SSA values are fixed. You cannot pass "whichever buffer
is not currently being filled by DMA" as an iteration argument without
explicitly building index arithmetic that the downstream passes will not
understand as buffer rotation.

#### Read-compute-write 3-stage pipeline collapses entirely

Adding a write-back DMA (output → DRAM) produces two loop-carried tokens:

```mlir
scf.for %i iter_args(%read_tok, %write_tok) {
  %next_write = async.execute { memref.copy %out_buf, %dram_out[%i-1] }
  %next_read  = async.execute { memref.copy %dram_in[%i], %in_buf }
  async.await %read_tok    // wait for this iteration's input
  npu.matmul_fp4x2 ...
  async.await %write_tok   // wait for output buffer to be free
  scf.yield %next_read, %next_write
}
```

The backend now sees two loop-carried dependency edges and must determine:

- That these correspond to two different DMA channels
- That `%read_tok` and `%write_tok` have different completion semantics
- That the output buffer lifecycle spans two iterations
- That `async.await %write_tok` before compute is wrong —
  it should be after the previous iteration's compute, not this one's

None of this is in the IR. The backend either serializes everything
conservatively (correct but slow) or attempts analysis that may be wrong.

#### What is actually lost at each lowering step

```
Source intent
  "issue DMA(i+1) at the start of iteration i,
   overlap it with compute(i),
   the two use separate DMA channels,
   buffer alternates between sram_a and sram_b"
        │
        ▼  scf.for + async
  loop-carried tokens + memref.copy regions
  [buffer rotation: gone — SSA cannot express it]
        │
        ▼  async lowering
  runtime token objects + copy calls
  [channel assignment: gone — async has no channel model]
        │
        ▼  bufferization
  concrete memref allocations
  [overlap intent: gone — now just sequential dependencies]
        │
        ▼  backend scheduling
  instruction DAG
  [timing: gone — DAG has no cycle model]
        │
        ▼  emission
  instructions in some order
  [pipeline structure: gone — reconstructed by scheduler, possibly wrong]
```

Each lowering step loses information that the next step would need
to make the right decision. By the time the backend scheduler runs,
the pipeline structure it is trying to reconstruct no longer exists
anywhere in the representation.

### C++ path

```cpp
// DMA primitive: move one tile from DRAM to SRAM
// Size is derived from the type — not a magic constant
template<typename TileT>
inline void dma_issue(TileT* sram_dst, TileT* dram_src) {
    constexpr size_t bytes = sizeof(TileT);  // 512 for WeightTile, always correct
    asm volatile("dma.issue %0, %1, %2"
                 : : "r"(sram_dst), "r"(dram_src), "i"(bytes));
}

inline void dma_wait() {
    asm volatile("dma.wait");
}

// Pipeline: double-buffer over a 2D tile grid
// Depth=2 → double buffer. Change to 3 for triple buffer.
// ComputeFn is injected at compile time — no indirect call.

template<int Depth, typename TileT, typename ComputeFn>
inline void matmul_pipeline(TileT** sram_bufs,   // Depth SRAM buffers
                             TileT*  dram_tiles,  // flat tile array
                             int     N,           // number of tiles
                             float*  act,
                             float*  out,
                             ComputeFn compute) {

    // Prologue: fill the pipeline
    unroll<Depth>([&](auto i) {
        dma_issue(sram_bufs[i], &dram_tiles[i]);
    });

    // Steady state: issue next tile, compute current tile, wait for DMA
    for (int i = Depth; i < N; i++) {
        dma_issue(sram_bufs[i % Depth], &dram_tiles[i]);  // (1) prefetch next
        compute(out, act, sram_bufs[(i - Depth) % Depth]); // (2) compute current
        dma_wait();                                         // (3) wait done
    }

    // Epilogue: drain the pipeline
    unroll<Depth>([&](auto i) {
        compute(out, act, sram_bufs[(N - Depth + i) % Depth]);
        dma_wait();
    });
}
```

Usage:

```cpp
// Weight matrix: 2×64 grid of fp4x2 tiles
WeightMatrix weights;  // Tensor<Tile<fp4x2, 32, 32>, 2, 64>

// SRAM double-buffer: 2 tiles × 512 bytes each
WeightTile sram[2];
WeightTile* sram_bufs[2] = { &sram[0], &sram[1] };

// Inject the matmul kernel at compile time
auto kernel = [](float* out, float* act, WeightTile* w)
              __attribute__((always_inline)) {
    matmul_tile<WeightTile>(out, act, w);
};

// Run the pipeline over all 128 tiles (2×64 grid, flattened)
matmul_pipeline<2>(sram_bufs, weights.data(), 128, act.data(), out.data(), kernel);
```

The DMA size is `sizeof(WeightTile)` — 512 bytes, derived from the type,
always correct. The prologue / steady-state / epilogue structure is written once
and reused for any tile type, any kernel, any depth.
The asm ordering guarantee means (1), (2), (3) appear in exactly that sequence
after template expansion. The timing intent is not in a comment — it is in the
structure of the code, and the compiler preserves it.

---

## Part 4: Compile-Time Queue — Flattening the Pipeline into a Static Sequence

The pipeline template in Part 3 still contains a runtime `for` loop in the
steady-state phase. For small, fixed tile counts this can be fully unrolled.
But there is a deeper point: the entire pipeline schedule — every DMA issue,
every compute, every wait, across prologue, steady state, and epilogue — can be
computed at compile time and stored as a flat constexpr array.

This is the compile-time queue pattern.

### Why this matters

MLIR's pipeline problem is structural, not accidental.

The MLIR design assumes that semantic information can be preserved through
a chain of dialect lowerings. In practice, each lowering is lossy:
the pass that handles `async` does not propagate DMA channel information
to the pass that handles `memref`, which does not propagate buffer rotation
information to the pass that handles `scf.for`, and so on.

The backend scheduler that ultimately decides instruction order sees
a dependency graph. It does not see a pipeline. It must reconstruct
the pipeline structure from the graph — but the graph no longer contains
the information needed to do this correctly:

```
What the programmer intended:    What the backend scheduler sees:

  issue DMA(i+1)                   node A: copy op
  overlap with compute(i)          node B: matmul op
  wait DMA(i+1)                    node C: await op
  [buffer alternates]              edge:   C depends on A
                                   edge:   B depends on previous C
                                   [buffer rotation: not in graph]
                                   [channel occupancy: not in graph]
                                   [overlap intent: not in graph]
                                   [issue timing: not in graph]
```

The scheduler must decide whether B and A can actually overlap on this hardware.
It has no hardware model. It either assumes they can (may be wrong) or assumes
they cannot (always slow).

The compile-time queue approach does not give the scheduler a graph to analyze.
It gives the scheduler a sequence that is already correct.
The scheduler's job becomes trivial: emit what you are given, in order.

### The schedule

```cpp
// PipelineSchedule<N, Depth>
// Computes the complete pipeline execution plan at compile time.
// N     — total number of tiles
// Depth — pipeline depth (2 = double buffer, 3 = triple buffer)

template<int N, int Depth>
struct PipelineSchedule {

    enum class Op { DMA_ISSUE, COMPUTE, DMA_WAIT };

    struct Slot {
        int tile_idx;
        int buf_idx;
        Op  op;
    };

    // Total slots: Depth (prologue) + (N-Depth)*3 (steady state) + Depth*2 (epilogue)
    static constexpr int total = Depth + (N - Depth) * 3 + Depth * 2;

    static constexpr auto build() {
        std::array<Slot, total> slots{};
        int s = 0;

        // Prologue: fill the pipeline
        for (int i = 0; i < Depth; i++)
            slots[s++] = {i, i % Depth, Op::DMA_ISSUE};

        // Steady state: issue next, compute current, wait
        for (int i = Depth; i < N; i++) {
            slots[s++] = {i,       i % Depth,         Op::DMA_ISSUE};
            slots[s++] = {i-Depth, (i - Depth)%Depth, Op::COMPUTE};
            slots[s++] = {i,       i % Depth,         Op::DMA_WAIT};
        }

        // Epilogue: drain the pipeline
        for (int i = 0; i < Depth; i++) {
            slots[s++] = {N-Depth+i, (N-Depth+i)%Depth, Op::COMPUTE};
            slots[s++] = {N-Depth+i, (N-Depth+i)%Depth, Op::DMA_WAIT};
        }

        return slots;
    }

    static constexpr auto schedule = build();
};
```

### Executing the schedule

```cpp
template<int N, int Depth, typename TileT, typename ComputeFn>
inline void pipeline_from_schedule(TileT** sram_bufs,
                                   TileT*  dram_tiles,
                                   float*  act,
                                   float*  out,
                                   ComputeFn compute) {
    using Sched = PipelineSchedule<N, Depth>;
    using Op    = typename Sched::Op;

    // unroll<M> expands to M consecutive compile-time-indexed calls.
    // Every slot index is a compile-time constant,
    // so every if constexpr branch is resolved before codegen.
    unroll<Sched::total>([&](auto I) {
        constexpr auto slot = Sched::schedule[I];

        if constexpr (slot.op == Op::DMA_ISSUE) {
            dma_issue(sram_bufs[slot.buf_idx], &dram_tiles[slot.tile_idx]);
        } else if constexpr (slot.op == Op::COMPUTE) {
            compute(out, act, sram_bufs[slot.buf_idx]);
        } else if constexpr (slot.op == Op::DMA_WAIT) {
            dma_wait();
        }
    });
}
```

### What the compiler sees after expansion

For `N=4, Depth=2` the unroll produces exactly this flat sequence — no loops,
no branches, no runtime decisions:

```asm
dma.issue  sram[0], dram[0]     ; prologue
dma.issue  sram[1], dram[1]     ; prologue

dma.issue  sram[0], dram[2]     ; steady: issue 2
npu.matmul sram[0], act, out    ; steady: compute 0
dma.wait                        ; steady: wait 2

dma.issue  sram[1], dram[3]     ; steady: issue 3
npu.matmul sram[1], act, out    ; steady: compute 1
dma.wait                        ; steady: wait 3

npu.matmul sram[0], act, out    ; epilogue: compute 2
dma.wait
npu.matmul sram[1], act, out    ; epilogue: compute 3
dma.wait
```

Every causal relationship that existed in the programmer's intent — issue before
compute, compute before wait, double-buffer index alternation — is present in
the final asm sequence. Nothing was lost. No pass had to reconstruct it.

### Comparison with MLIR

In MLIR, reconstructing this sequence from a DAG of `async` tokens and
`memref.copy` operations is the job of the backend scheduler. The scheduler
must infer the pipeline structure, the buffer rotation, and the issue timing
from the dependency graph — information that was explicit at the source level
but was compressed away during lowering.

In the compile-time queue approach, the scheduler has nothing to do.
The sequence is already correct. The compiler's only job is to emit it
in the order it was given — which is exactly the guarantee c4c provides.

---

## Summary

| | MLIR | c4c C++ |
|---|---|---|
| **fp4x2 type** | Define new dialect, type class, parser, printer | `struct fp4x2 { uint8_t data; }` |
| **Tile layout** | Implicit in memref strides, lost after lowering | `Tile<fp4x2, 32, 32>` — size in type, always correct |
| **Tiled tensor** | Strided memref, tile boundary not in type | `Tensor<WeightTile, 2, 64>` — shape in type |
| **Opcode selection** | Custom op + lowering pass per type | `consteval matmul_opcode<WeightTile>()` |
| **DMA primitive** | `memref.copy` — backend decides implementation | `dma_issue<TileT>()` — direct asm, size from type |
| **DMA size** | Derived from strides at lowering time, can be wrong | `sizeof(TileT)` — compile-time constant, always correct |
| **DMA lifecycle** | Not representable — `async.token` has no channel, latency, or lock model | Explicit: `dma_issue` / `dma_wait` as ordered asm |
| **DMA channel model** | Not in IR — backend decides, no guidance | Programmer controls: separate issue/wait per channel |
| **Buffer rotation** | Not expressible in SSA — requires implicit index arithmetic | `buf_idx % Depth` — explicit in every slot |
| **Loop-carried pipeline** | `scf.for iter_args` cannot express buffer swap — SSA is fixed | Compile-time queue: slot array carries buf_idx per stage |
| **3-stage read-compute-write** | Two loop-carried tokens, backend must infer channel separation | Two `Op` variants in schedule, channel assignment explicit |
| **Pipeline structure** | Re-implemented per kernel as affine loops | `pipeline<Depth, TileT, ComputeFn>` — written once |
| **Timing intent** | Lost at each lowering step — not recoverable by backend | Preserved — asm ordering is a guarantee |
| **Backend scheduler role** | Must reconstruct pipeline from dependency graph — lossy | Trivial — sequence is already correct, emit in order |
| **Adding a new tile type** | New type, new op, new lowering pass | New struct + one `consteval` specialization |
| **Pipeline schedule** | Reconstructed from DAG — intent already lost | `constexpr PipelineSchedule` — flat, complete at compile time |
| **Correctness guarantee** | Formal on IR model — hardware behavior not in model | Empirical — FPGA regression + cycle benchmark |

The MLIR path requires approximately 355 lines of infrastructure code
before the first line of kernel logic is written.
That infrastructure defines the type, registers it, parses and prints it,
lowers it through the pass pipeline, and models it in the scheduler.
It does not include op definitions, op lowering passes, or DMA handling.

The c4c C++ path requires:

```
struct fp4x2 { uint8_t data; };               1 line
Tile<T, R, C>                                 8 lines
Tensor<T, Dims...>                            6 lines
consteval vload_opcode<WeightTile>()          1 line
─────────────────────────────────────────────
Total                                        16 lines  full type + opcode
```

The deeper issue is not the amount of work. It is that the MLIR lowering pipeline
is a one-way compressor: timing intent, tile boundaries, and DMA semantics enter
at the top and do not survive to the bottom. The hardware does not run MLIR.
It runs instructions, in order, at specific cycle boundaries.
C++ with inline asm is the closest language to that reality.

The question is not "why avoid MLIR."
The question is: you are writing C++ either way.
In MLIR you write C++ to build infrastructure that generates IR that gets lowered
into something the hardware can run — losing information at every step.
In c4c you write C++ that the hardware runs directly.