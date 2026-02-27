# C4 Language Specification
## A Lambda Calculus Foundation for Systems Programming

**Version**: 0.2 (Draft)  
**Design principle**: 編譯器實現用.c4，使用者看到的是C/C++ liked語法

---

## 定位

```
使用者視角：
  寫C/C++ liked語法
  熟悉的class, template, operator
  不需要理解底層

編譯器視角：
  .c4實現stdlib和runtime
  底層是cast table + sum type + DAG rewrite
  不依賴LLVM、TableGen、libstdc++

類比CUDA：
  語法像C++，執行模型完全不同
  std::vector不能在device用，但有自己的容器

C4對C++：
  語法像C++，型別系統底層完全不同
  std::不能用，但有c4::std實作相同介面
```

---

## 檔案類型與互操作

```
.c   → 純C，完全兼容，不做任何限制
.cpp → 純C++，完全兼容，不做任何限制
.c4  → C4，compiler實現語言，底層語意不同

三種檔案可以在同一個專案中互相link，共享C ABI
```

### Link兼容性

```
.c   ↔ .c4  → 完全互通，C ABI
.cpp ↔ .c4  → 透過extern "C"邊界
.c   ↔ .cpp → 同現在，不變
```

```c4
// .c4呼叫.c
extern "C" void* malloc(size_t n);

// .c4呼叫.cpp
extern "C" void cpp_function(int x);

// .c4函數預設export為C ABI，.cpp可直接呼叫
```

---

## 架構層次

```
┌─────────────────────────────────────┐
│  User Code                          │
│  C / C++ liked syntax               │
│  class, template<>, operator        │
├─────────────────────────────────────┤
│  c4::std  (Standard Library)        │
│  用.c4實現                           │
│  vector<T>, map<K,V>, Optional<T>   │
│  Result<T>, string, ...             │
├─────────────────────────────────────┤
│  C4 Core  (.c4)                     │
│  sum type, match, cast table        │
│  closure, region, own               │
├─────────────────────────────────────┤
│  C Runtime                          │
│  malloc/free, C ABI                 │
│  Stage 0 bootstrap compiler (純C)   │
└─────────────────────────────────────┘
```

---

## C4核心語意（.c4實現層）

compiler工程師看到的東西，不是一般使用者。

### Sum Type

```c4
module c4.std.option;

type Option<T> = Some(T) | None
type Result<T> = Ok(T) | Err(str)

// 展開為tagged union（C ABI透明）
// struct Option { enum { SOME, NONE } tag; union { T val; } data; };
```

### Pattern Matching

```c4
fn unwrap<T>(Option<T> opt) -> T {
    match opt {
        Some(v) => v,
        None    => panic("unwrap on None"),
    }
}
// 展開為switch(opt.tag)，O(1)，無vtable
// 所有arm未覆蓋時編譯錯誤（exhaustiveness check）
```

### Cast Table

```c4
// 全局唯一，key = (from_type_id, to_type_id)
register cast<IRNode, MachineInstr> = (n) => match n {
    Add(l, r) => MachineInstr(ADD, l, r),
    Lit(v)    => MachineInstr(MOV, v),
    _         => null,
}

let instr = cast<MachineInstr>(node)  // O(1) hash lookup
```

### Closure

```c4
// runtime = fat pointer (fn_ptr, env_ptr)
int base = 10;
let add_base = (int x) => x + base;  // base複製進env

// Strategy組合子
let bottomUp = (rule) => (node) => rule(map(bottomUp(rule), node))
let fixpoint = (rule) => (node) => {
    let r = rule(node);
    r == node ? node : fixpoint(rule)(r)
}
```

### 記憶體管理

```c4
// Region：pass的天然記憶體模式
region {
    Node* ast = parse(input);
    Node* ir  = lower(ast);
}  // 一次釋放

// Own：獨佔所有權，離開scope自動釋放
own<Node> n = alloc(Node, Lit(5));

// Unsafe：裸指標，.c/.cpp整體視為unsafe
unsafe {
    int* p = (int*)malloc(sizeof(int));
    free(p);
}
```

---

## User Syntax（C/C++ liked層）

使用者寫這一層，compiler展開成C4 core。

### Class → Type + Register

使用者寫：
```cpp
template<typename T>
class Optional {
    T value;
    bool valid;
public:
    Optional(T v) : value(v), valid(true) {}
    Optional()    : valid(false) {}
    T& operator*()  { return value; }
    operator bool() { return valid; }
};
```

Compiler展開成：
```c4
type Optional<T> = Some(T) | None

register (*)<Optional<T>>  = (o) => match o { Some(v) => v, None => panic() }
register bool<Optional<T>> = (o) => match o { Some(_) => true, None => false }
```

### 繼承 → Cast的自動註冊

```cpp
class ColorPoint extends Point { int color; }
```

展開成：
```c4
type ColorPoint = ColorPoint(int x, int y, int color)

register cast<ColorPoint, Point> = (cp) => {
    let ColorPoint(x, y, _) = cp;
    Point(x, y)
}
// Point的所有method透過cast自動可用，無vtable
```

### 標準容器 → c4::std

```cpp
std::vector<int> v;   // 編譯器對應到c4::std::vector
v.push_back(1);       // cast table lookup
int x = v[0];         // cast table lookup
```

底層是sum type + cast table，不是libstdc++。

---

## 不兼容清單

### 不支援，有替代方案

```
C++                    C4替代方案
──────────────────────────────────────
typeid()          →    c4::type_id()
dynamic_cast<T>   →    cast<T>(value)
std::              →    c4::std::
虛擬繼承           →    sum type
throw / catch      →    Result<T> + match
RTTI              →    hash-based type_id
```

### 支援且語意相同

```
static_cast<T>        完全支援
reinterpret_cast<T>   unsafe block內合法
struct / union        完全兼容，C ABI透明
函數指標              完全兼容
extern "C"            完全支援
#include              仍然合法（兼容舊code）
```

---

## Type ID系統

```
type_id = hash(module_path + "." + type_name + "|" + structural_layout)
```

遞迴型別：
```
hash("c4.ir.Node|Add(->c4.ir.Node,->c4.ir.Node)|Lit(int32)")
→ uint64
```

- Intrinsic型別（int, float, ptr）的type_id固定，跨編譯單元一致
- Cast table的key是(from_type_id, to_type_id)，O(1) hash lookup
- 跨lib的同名型別透過module path的hash自動區分
- 跨語言邊界（extern "C"）型別資訊消失，由使用者負責對應

---

## Operator Overloading

```c4
// .c4實現層定義
register (+)<Tensor, Tensor>  -> Tensor = tensor_add
register (*)<Tensor, f32>     -> Tensor = tensor_scale
register (|>)<Pass, Pass>     -> Pass   = compose
register ([])<Vector<T>, int> -> T      = vector_index
```

```cpp
// 使用者層
Tensor c = a + b;    // → tensor_add（cast table）
Tensor d = c * 0.5f; // → tensor_scale
auto x   = v[0];     // → vector_index
```

歧義（同型別組合多個實作）產生編譯錯誤，不做隱式overload resolution。

---

## DAG Rewrite：取代C++ + TableGen

.c4的核心用途是實現compiler前後端，消除TableGen依賴：

```c4
module compiler.backend.lower;

// Instruction selection = cast registration，取代TableGen的.td
register cast<ir.Add, machine.Instr> = (n) => match n {
    ir.Add(ir.Reg(a), ir.Reg(b))   => machine.ADD(a, b),
    ir.Add(ir.Reg(a), ir.Lit(imm)) => machine.ADDI(a, imm),
    ir.Add(ir.Lit(a), ir.Lit(b))   => machine.MOV(a + b),
}

// Optimization = rewrite rule組合，取代LLVM Pass
let optimize: Pass = fixpoint(bottomUp((n) => match n {
    ir.Add(x, ir.Lit(0)) => x,
    ir.Mul(_, ir.Lit(0)) => ir.Lit(0),
    ir.Mul(x, ir.Lit(1)) => x,
    n                    => n,
}))

// Pipeline = closure組合，取代PassManager
fn compile(str src) -> Binary {
    src
    |> parse
    |> cast<ir.Module>
    |> optimize
    |> cast<machine.Module>
    |> regalloc
    |> emit
}
```

---

## Bootstrap路徑

```
Stage 0（純python，幾千行）：
  實現.c to .ll完整功能 要過unit test
  目前是做tests/c-testsuite
  未來要包llvm的C unit test
  暫且依賴clang preprocessor以及backend (llc)


Stage 1
  基於stage 0 C compiler 拓展.c4 新特性unittest
  sum type展開為tagged union
  match展開為switch on tag
  cast table = hash map實現
  跑通unit test

Stage 2（自編譯驗證）：
  std先串接cstdio
  用.c4重寫整個.c4編譯器前端
  c4編譯器編譯自己
  bit-for-bit相同 → 自編譯完成

Stage 3
  完善整個toolchain
  實現編譯器後端與preprocessor
  實現.c4 - .c cross link
  不依賴於clang/llvm

Stage 4
  用.c4實現部分C++ template功能 以及部分c++ stdlib
  實現.c4 .c .cpp cross link
  
```

---

## 設計原則

```
使用者不需要知道：
  sum type, cast table, type_id（實現細節）

使用者需要知道：
  std::      → c4::std::
  typeid()   → c4::type_id()
  dynamic_cast → cast<T>()
  throw/catch  → Result<T>
  其他C++語法幾乎照常

.c4解決的根本問題：
  C++和TableGen的耦合 → 全部用.c4的DAG rewrite表達
  前後端設計語言統一 → 不再需要兩套工具
  使用者介面不變    → C/C++ liked語法，學習成本低
```