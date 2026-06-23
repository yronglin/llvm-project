# PNU `std::initializer_list` Merging Design

本文档是当前分支中 `std::initializer_list` 静态 backing array 合并实现的中英双语设计说明。它解释目标、非目标、设计取舍、实现路径、测试覆盖和后续工作。

This document is a bilingual design note for the current branch implementation of merging static backing arrays for `std::initializer_list`. It covers the goals, non-goals, design tradeoffs, implementation details, test coverage, and future work.

Primary external reference: Arthur O'Dwyer, ["ELF's ways to combine potentially non-unique objects"](https://quuxplusone.github.io/blog/2026/05/05/potentially-nonunique-strategies/), 2026-05-05.

## 1. Summary / 摘要

中文：

这个分支实现的核心不是 C++26 reflection，也不是完整的 `std::define_static_array`。当前目标更窄：让 Clang 为可静态化的 `std::initializer_list` backing array 在 ELF 上发射一种新的 potentially non-unique object 区域 `.rodata.pnu`，再让 lld 对这些对象按字节内容做跨 translation unit 的 exact merge 和 tail merge。

English:

The branch does not implement C++26 reflection, nor a complete `std::define_static_array`. Its narrower goal is to make Clang emit statically materialized `std::initializer_list` backing arrays into a new ELF area, `.rodata.pnu`, and to make lld merge those objects by byte contents across translation units, including exact merging and tail merging.

中文：

实现上复用了 lld 现有的 `MergeInputSection` / `MergeSyntheticSection` 框架，但 `.rodata.pnu` 的 piece 边界不来自 `sh_entsize`，也不来自 NUL 结尾字符串扫描，而是来自 `.symtab` 中指向该 section 的 `STT_OBJECT` symbol 的 `[st_value, st_value + st_size)` 范围。

English:

The implementation reuses lld's existing `MergeInputSection` / `MergeSyntheticSection` machinery. However, `.rodata.pnu` piece boundaries are not derived from `sh_entsize`, and not from NUL-terminated string scanning. They come from `STT_OBJECT` symbols in `.symtab` whose ranges are `[st_value, st_value + st_size)` within the `.rodata.pnu` section.

中文：

当前状态可以概括为：

- Clang：对无 relocation、默认 address space、ELF 目标上的静态 initializer-list backing array，发射 `internal constant ... section ".rodata.pnu"`。
- MC：对 `.rodata.pnu` 中的局部对象符号，保留 relocation 的 symbol 引用，不把它折成 section symbol + addend。
- lld：把 `.rodata.pnu` 当作特殊 merge input section，按对象符号范围切 piece，使用 tail-merging synthetic section 合并内容并修正符号地址。

English:

The current status is:

- Clang: for relocation-free static initializer-list backing arrays in the default address space on ELF targets, emit `internal constant ... section ".rodata.pnu"`.
- MC: preserve relocations against local object symbols in `.rodata.pnu`, instead of rewriting them to section-symbol relocations plus addends.
- lld: treat `.rodata.pnu` as a special merge input section, split it by object-symbol ranges, use a tail-merging synthetic section to merge contents, and repair symbol addresses.

## 2. Problem Statement / 问题定义

中文：

`std::initializer_list<T>` 的对象通常只是一个 pointer + size pair。真正的元素数据放在一个 backing array 中。对于常量 initializer-list，例如 `{1, 2, 3}`，这个 backing array 可以被提升到静态只读存储。C++ 标准允许这类 backing array 作为 potentially non-unique object：实现可以让等价的 backing arrays 共享同一段存储，也可以让一个 backing array 指向另一个更大 backing array 的尾部。

English:

A `std::initializer_list<T>` object is usually represented as a pointer plus a size. The elements live in a backing array. For constant initializer lists such as `{1, 2, 3}`, the backing array can be promoted to static read-only storage. The C++ standard permits such backing arrays to be potentially non-unique objects: an implementation may make equivalent backing arrays share storage, or may point one backing array into the tail of a larger one.

中文：

我们想达到的效果是：

```c++
void sink(std::initializer_list<int>);

void a() {
  sink({1, 2, 3});
  sink({2, 3});
}

void b() {
  sink({1, 2, 3});
}
```

在链接后，只保留一个 `{1, 2, 3}` 的字节序列：

```text
01 00 00 00  02 00 00 00  03 00 00 00
```

其中两个 `{1, 2, 3}` 的 backing array 地址相同，而 `{2, 3}` 的 backing array 地址是 `{1, 2, 3}` 起点加 4。

English:

The intended result is:

```c++
void sink(std::initializer_list<int>);

void a() {
  sink({1, 2, 3});
  sink({2, 3});
}

void b() {
  sink({1, 2, 3});
}
```

After linking, the output should keep only one byte sequence for `{1, 2, 3}`:

```text
01 00 00 00  02 00 00 00  03 00 00 00
```

Both `{1, 2, 3}` backing arrays should resolve to the same address, while the `{2, 3}` backing array should resolve to the address of `{1, 2, 3}` plus 4.

## 3. Non-Goals / 非目标

中文：

这个分支有意不实现以下内容：

- 不实现 C++26 reflection。
- 不实现 `std::define_static_array` 的语义。
- 不试图把所有 C++ potentially non-unique objects 建模完整。
- 不修改非 ELF object format。
- 不要求系统 linker 都理解 `.rodata.pnu`。
- 不把 inline variables、template parameter objects 或其他必须按语言规则保证唯一或按名字合并的实体放进 `.rodata.pnu`。

English:

This branch intentionally does not implement:

- C++26 reflection.
- The semantics of `std::define_static_array`.
- A complete model for all C++ potentially non-unique objects.
- Non-ELF object formats.
- Support in every system linker.
- Emission of inline variables, template parameter objects, or other entities requiring language-mandated uniqueness or name-based merging into `.rodata.pnu`.

中文：

这里的核心边界是：`std::initializer_list` backing arrays 允许被合并或重叠；inline variables 不能仅因为内容相同就合并；`define_static_array` 如果标准要求所有相同调用必须产生同一对象，则不能依赖可选的内容合并机制。

English:

The key boundary is: `std::initializer_list` backing arrays may be merged or overlapped; inline variables must not be merged merely because their contents are equal; and if `define_static_array` requires every identical invocation to produce the same object, it cannot rely on an optional content-merging mechanism.

## 4. Design Reasoning / 设计推理

### 4.1 Why not only compiler-level merging? / 为什么不只做编译器内合并？

中文：

Clang 可以在单个 translation unit 内复用完全相同的 `llvm::Constant`，当前分支已经通过 `StaticInitListBackingArrayMap` 做这件事。这能解决同一个 `.cpp` 文件中重复出现的 `{1, 2, 3}`，但解决不了跨 translation unit 的重复，也很难在前端中维护复杂的尾部合并搜索。

English:

Clang can reuse the same `llvm::Constant` within a single translation unit, and this branch does that through `StaticInitListBackingArrayMap`. This handles repeated `{1, 2, 3}` in one `.cpp` file, but it cannot handle duplicates across translation units, and it would be awkward to maintain complex tail-merging searches in the frontend.

中文：

链接器看到所有 object files，天然拥有更完整的信息。把“跨 TU 的 exact/tail merge”放在 lld 中，可以避免把前端变成一个静态数据压缩器。

English:

The linker sees all object files and naturally has more complete information. Putting cross-TU exact and tail merging in lld avoids turning the frontend into a static-data compressor.

### 4.2 Why not plain `SHF_MERGE`? / 为什么不用普通 `SHF_MERGE`？

中文：

普通 `SHF_MERGE` section 按固定 `sh_entsize` 把内容切成等长元素，然后只合并完整元素。它适合字符串字面量和固定大小常量池，但不适合 `{1,2,3}` 与 `{2,3}` 这种不同长度对象的尾部重叠。它还要求编译器提前选择 bucket，例如 8 字节、16 字节、32 字节；不同 bucket 之间不会互相合并。

English:

A normal `SHF_MERGE` section splits contents into fixed-size records using `sh_entsize`, and merges only whole records. It works for string literals and fixed-size constant pools, but not for tail-overlapping objects of different lengths such as `{1,2,3}` and `{2,3}`. It also requires the compiler to preselect a bucket, such as 8, 16, or 32 bytes; different buckets do not merge with each other.

中文：

本分支前面做过一个 fallback：在非 `.rodata.pnu` 路径上，把小的无 relocation backing array padding 到 4 字节以上的 2 的幂大小，以便普通 `SHF_MERGE` 可以合并更多常量。但 ELF + lld 的目标路径不再依赖这点，而是使用 `.rodata.pnu` 获得对象级 piece 边界。

English:

Earlier in this branch, a fallback path padded small relocation-free backing arrays to a power-of-two size of at least 4 bytes, making normal `SHF_MERGE` more effective. The ELF + lld target path no longer relies on that fallback; it uses `.rodata.pnu` to provide object-level piece boundaries instead.

### 4.3 Why not COMDAT or hash-named groups? / 为什么不用 COMDAT 或基于 hash 的 group？

中文：

COMDAT / `SHF_GROUP` 是按名字合并，不是按内容合并。它适合 inline functions、inline variables、模板实例化等需要“同名定义合并”的实体。对于 initializer-list backing array，我们需要按字节内容合并，且希望能把一个对象并入另一个对象的尾部。用内容 hash 生成 COMDAT group 名可以模拟内容合并，但会增加大量 section 和 symbol，也会把 hash collision 变成 ABI 和正确性问题。

English:

COMDAT / `SHF_GROUP` merges by name, not by contents. It is appropriate for inline functions, inline variables, and template instantiations where same-name definitions must be merged. For initializer-list backing arrays, we need byte-content merging, and we want to place one object into the tail of another. Creating COMDAT group names from content hashes can simulate content merging, but it creates many sections and symbols, and turns hash collisions into ABI and correctness problems.

### 4.4 Why `.rodata.pnu`? / 为什么引入 `.rodata.pnu`？

中文：

`.rodata.pnu` 是一个更直接的 contract：编译器把可以作为 potentially non-unique object 的静态只读数据放进这个 section，并通过 object symbols 告诉 linker 每个对象的起点和大小。linker 随后可以自由地 exact-merge 或 tail-merge 这些对象，只要输出地址仍然指向字节内容相同的对象范围。

English:

`.rodata.pnu` is a more direct contract: the compiler places static read-only data that may be treated as a potentially non-unique object into this section, and object symbols tell the linker the start and size of each object. The linker may then exact-merge or tail-merge those objects, as long as every resulting address still points at an object range with identical bytes.

中文：

Arthur O'Dwyer 的文章提出过一个可能的方向：把 PNU initializers 放到统一 section，再用额外 metadata 描述符号范围。当前分支采用了这个思路的最小实现：不新增 `.pnu_symtab`，而是复用 ELF 已有的 symbol table。这样可以减少格式改动，同时足够服务 Clang 当前能发射的 initializer-list backing arrays。

English:

Arthur O'Dwyer's article suggests a possible direction: place PNU initializers in a common section, then use additional metadata to describe symbol ranges. This branch implements the minimal form of that idea: it does not add a `.pnu_symtab`; it reuses the existing ELF symbol table. This minimizes object-format changes while supporting the initializer-list backing arrays that Clang can currently emit.

### 4.5 Why remove `unnamed_addr` for PNU globals? / 为什么 PNU global 不能保留 `unnamed_addr`？

中文：

这是当前分支中一个关键修正。LLVM IR 的 `unnamed_addr` 会让后端把某些常量当作普通可合并常量处理。实际 smoke test 中，16 字节或 8 字节的 PNU 对象被 MC 放进了普通 `SHF_MERGE` section，例如 `.rodata.pnu,"aM",@progbits,8`，导致 `{1,2,3}` 和 `{2,3}` 不在同一个 PNU merge domain 中，lld 无法做目标中的 tail merge。

English:

This is a key correction in the branch. LLVM IR `unnamed_addr` lets the backend treat some constants as ordinary mergeable constants. In the smoke test, 16-byte or 8-byte PNU objects were emitted into ordinary `SHF_MERGE` sections such as `.rodata.pnu,"aM",@progbits,8`. That put `{1,2,3}` and `{2,3}` in different merge domains, preventing lld from performing the desired tail merge.

中文：

因此，对 `.rodata.pnu` 路径，Clang 发射 `internal constant` 但不设置 `unnamed_addr`。这样 MC 会把所有 PNU 对象保留在普通 alloc-only `.rodata.pnu` 中，由 lld 的 PNU 逻辑统一切 piece 和合并。

English:

Therefore, on the `.rodata.pnu` path, Clang emits `internal constant` but does not set `unnamed_addr`. This keeps all PNU objects in a normal alloc-only `.rodata.pnu` section, allowing lld's PNU logic to split and merge them consistently.

## 5. Clang Implementation / Clang 实现

### 5.1 Frontend entry point / 前端入口

中文：

入口在 `clang/lib/CodeGen/CGExpr.cpp`：

- `CodeGenFunction::tryEmitStaticInitListBackingArray`
- 调用位置：`CodeGenFunction::EmitMaterializeTemporaryExpr`

它只处理 `MaterializeTemporaryExpr` 中标记为 initializer-list backing array 的临时对象。当前允许的 storage duration 是 `SD_FullExpression` 和 `SD_Automatic`；如果是 `SD_Static` 或 `SD_Thread`，仍走原有 global temporary 路径。

English:

The entry point is in `clang/lib/CodeGen/CGExpr.cpp`:

- `CodeGenFunction::tryEmitStaticInitListBackingArray`
- Called from `CodeGenFunction::EmitMaterializeTemporaryExpr`

It only handles `MaterializeTemporaryExpr` nodes marked as initializer-list backing arrays. The currently accepted storage durations are `SD_FullExpression` and `SD_Automatic`; `SD_Static` and `SD_Thread` continue to use the existing global-temporary path.

中文：

静态化条件包括：

- subexpression type 必须是 array type。
- array type 必须满足 `isConstantStorage(..., ExcludeCtor=true, ExcludeDtor=false)`。
- `ConstantEmitter::tryEmitAbstractForMemory` 必须能生成 `llvm::Constant`。

English:

The static-materialization conditions include:

- The subexpression type must be an array type.
- The array type must satisfy `isConstantStorage(..., ExcludeCtor=true, ExcludeDtor=false)`.
- `ConstantEmitter::tryEmitAbstractForMemory` must produce an `llvm::Constant`.

### 5.2 Backing array creation / Backing array 创建

中文：

实际创建 global 的逻辑在 `clang/lib/CodeGen/CodeGenModule.cpp`：

- `CodeGenModule::EmitStaticInitListBackingArray`
- 状态缓存：`CodeGenModule::StaticInitListBackingArrayMap`

这个 map 的 key 是 `llvm::Constant *Init`，value 是对应的 `llvm::GlobalVariable *`。它保证同一个 translation unit 内相同 constant 对象只发射一次 backing array。

English:

The global creation logic is in `clang/lib/CodeGen/CodeGenModule.cpp`:

- `CodeGenModule::EmitStaticInitListBackingArray`
- Cache: `CodeGenModule::StaticInitListBackingArrayMap`

The map key is `llvm::Constant *Init`, and the value is the corresponding `llvm::GlobalVariable *`. It ensures that the same constant object in one translation unit emits only one backing array.

中文：

PNU 路径的判定条件是：

```c++
bool UsePnuSection = AddrSpace == LangAS::Default &&
                     getTarget().getTriple().isOSBinFormatELF() &&
                     !Init->needsRelocation();
```

也就是说：

- 只处理默认 address space。
- 只处理 ELF。
- 只处理无 relocation 的常量 initializer。

English:

The PNU path is selected by:

```c++
bool UsePnuSection = AddrSpace == LangAS::Default &&
                     getTarget().getTriple().isOSBinFormatELF() &&
                     !Init->needsRelocation();
```

That means:

- Default address space only.
- ELF only.
- Relocation-free constant initializers only.

中文：

PNU global 的发射形态是：

```llvm
@.init.list = internal constant [N x T] ..., section ".rodata.pnu", align A
```

重要属性：

- 使用 `internal` linkage，而不是 `private`，以便 object symbol 进入 ELF symbol table，供 lld 发现 piece range。
- 不设置 `unnamed_addr`，避免 MC 把它分类进普通 `SHF_MERGE` 常量 section。
- 保留真实 object size，避免 power-of-two padding 改变 PNU piece 的范围。

English:

The PNU global shape is:

```llvm
@.init.list = internal constant [N x T] ..., section ".rodata.pnu", align A
```

Important properties:

- It uses `internal` linkage, not `private`, so the object symbol is present in the ELF symbol table for lld to discover the piece range.
- It does not set `unnamed_addr`, preventing MC from classifying it as an ordinary `SHF_MERGE` constant section.
- It keeps the real object size, avoiding power-of-two padding in the PNU piece range.

中文：

非 PNU fallback 仍然使用 `private unnamed_addr`。对于小的无 relocation 常量，如果 size 在当前 heuristic 范围内，会 padding 到适合普通 merge section 的大小。这是为了非 ELF 或不使用 PNU 路径时仍能获得一些常量合并收益。

English:

The non-PNU fallback continues to use `private unnamed_addr`. For small relocation-free constants, it may pad the stored initializer to a size suitable for normal merge sections. This preserves some constant-merging benefit on non-ELF targets or paths that do not use PNU.

## 6. MC / Object Writer Implementation / MC 与 Object Writer 实现

中文：

相关修改在 `llvm/lib/MC/ELFObjectWriter.cpp` 的 `ELFObjectWriter::useSectionSymbol`。

普通 mergeable section 中，MC 有时会把对局部符号的 relocation 改写成对 section symbol 的 relocation，加上 addend。这个优化通常可以减少 symbol table 压力。但是 `.rodata.pnu` 的 piece 边界正是由局部 `STT_OBJECT` symbol 提供的。如果 relocation 从 object symbol 变成 section symbol + addend，lld 在某些 relocation 形式下需要反推出原本的 piece，容易出错。

English:

The relevant change is in `llvm/lib/MC/ELFObjectWriter.cpp`, in `ELFObjectWriter::useSectionSymbol`.

For ordinary mergeable sections, MC may rewrite relocations against local symbols into relocations against a section symbol plus an addend. That optimization can reduce symbol-table pressure. For `.rodata.pnu`, however, piece boundaries are defined by local `STT_OBJECT` symbols. If a relocation is rewritten from an object symbol into a section-symbol relocation plus an addend, lld would have to infer the original piece in some relocation forms, which is fragile.

中文：

因此，当前规则是：

```c++
if (Sec.getName() == ".rodata.pnu")
  return false;
```

这表示：对 `.rodata.pnu` 中的符号，不用 section symbol 替代，保留 relocation 的原始 object symbol。

English:

Therefore, the current rule is:

```c++
if (Sec.getName() == ".rodata.pnu")
  return false;
```

This means: for symbols in `.rodata.pnu`, do not replace them with section-symbol relocations; preserve the original object symbol in the relocation.

## 7. lld Implementation / lld 实现

### 7.1 Section recognition / Section 识别

中文：

`lld/ELF/InputFiles.cpp` 中，非 relocatable output 下，如果输入 section 名为 `.rodata.pnu` 且非空，就创建 `MergeInputSection`：

```c++
if (name == ".rodata.pnu" && !ctx.arg.relocatable) {
  if (sec.sh_flags & SHF_WRITE)
    error(...);
  if (sec.sh_size != 0)
    return makeThreadLocal<MergeInputSection>(*this, sec, name);
}
```

English:

In `lld/ELF/InputFiles.cpp`, for non-relocatable output, a non-empty input section named `.rodata.pnu` becomes a `MergeInputSection`:

```c++
if (name == ".rodata.pnu" && !ctx.arg.relocatable) {
  if (sec.sh_flags & SHF_WRITE)
    error(...);
  if (sec.sh_size != 0)
    return makeThreadLocal<MergeInputSection>(*this, sec, name);
}
```

中文：

如果 `.rodata.pnu` 是 writable，lld 报错。PNU 的语义要求对象是只读静态数据；写入一个被合并或重叠的对象会导致不可接受的别名行为。

English:

If `.rodata.pnu` is writable, lld reports an error. PNU semantics require read-only static data; writing to an object that may be merged or overlapped would create unacceptable aliasing behavior.

### 7.2 Piece splitting / Piece 切分

中文：

`lld/ELF/InputSection.cpp` 新增了 `MergeInputSection::splitPnuPieces`。它扫描当前 object file 的 symbols：

- 只接受 `Defined` symbol。
- symbol 必须属于当前 `.rodata.pnu` section。
- 跳过 section symbol。
- 跳过 size 为 0 的 symbol。
- 使用 `[d->value, d->value + d->size)` 作为 piece range。

English:

`lld/ELF/InputSection.cpp` adds `MergeInputSection::splitPnuPieces`. It scans the symbols of the current object file:

- Only `Defined` symbols are accepted.
- The symbol must belong to the current `.rodata.pnu` section.
- Section symbols are skipped.
- Zero-size symbols are skipped.
- The piece range is `[d->value, d->value + d->size)`.

中文：

然后它对 ranges 做排序和去重，并检查：

- range 不能越过 section 内容。
- ranges 不能互相重叠。
- offset 不能超过 32-bit piece offset 编码范围。

如果 section 有内容但没有 object symbol，当前 fallback 是把整个 section 当成一个 piece。这使手写 assembly 或非 Clang producer 不至于完全不可用，但 Clang 路径应当总是产生 object symbol。

English:

It then sorts and uniquifies the ranges, and checks:

- A range must not extend outside the section contents.
- Ranges must not overlap each other.
- Offsets must fit in the 32-bit piece-offset encoding.

If the section has contents but no object symbol, the current fallback treats the whole section as one piece. This keeps hand-written assembly or non-Clang producers usable, but the Clang path should always produce object symbols.

### 7.3 Tail merging / 尾部合并

中文：

`lld/ELF/OutputSections.cpp` 中，`.rodata.pnu` 选择 `MergeTailSection`：

```c++
if (name == ".rodata.pnu" || ((flags & SHF_STRINGS) && ctx.arg.optimize >= 2))
  return make<MergeTailSection>(...);
```

`MergeTailSection` 使用 `StringTableBuilder::RAW`。虽然名字里有 string table，但 RAW mode 处理的是任意字节序列，不要求 NUL 结尾。它能把一个 byte string 放进另一个 byte string 的尾部，因此适合 `{2,3}` 合并到 `{1,2,3}` 的后 8 字节。

English:

In `lld/ELF/OutputSections.cpp`, `.rodata.pnu` selects `MergeTailSection`:

```c++
if (name == ".rodata.pnu" || ((flags & SHF_STRINGS) && ctx.arg.optimize >= 2))
  return make<MergeTailSection>(...);
```

`MergeTailSection` uses `StringTableBuilder::RAW`. Despite the name, RAW mode handles arbitrary byte sequences and does not require NUL termination. It can place one byte string into the tail of another byte string, making it suitable for merging `{2,3}` into the last 8 bytes of `{1,2,3}`.

中文：

PNU section 与普通 non-string `SHF_MERGE` 的关键区别是：普通路径使用固定 `entsize`，而 PNU 路径使用每个 object symbol 的真实 size。因此不同长度的对象可以在同一个 merge domain 中比较和 tail-merge。

English:

The key difference from ordinary non-string `SHF_MERGE` is: the ordinary path uses fixed `entsize`, while the PNU path uses the real size of each object symbol. Therefore objects of different lengths can be compared and tail-merged in the same merge domain.

### 7.4 Symbol value repair / 符号值修正

中文：

`lld/ELF/SyntheticSections.cpp` 的 `splitSections` 在 `splitIntoPieces()` 之后，对指向 merge sections 的 `Defined` symbols 做预解析。它把 symbol value 编码为：

```text
((pieceIdx + 1) << mergeValueShift) | intraPieceOffset
```

后续 `getParentOffset` 可以 O(1) 找到 piece 的 output offset，再加上 piece 内偏移，得到最终地址。

English:

In `lld/ELF/SyntheticSections.cpp`, after `splitIntoPieces()`, `splitSections` pre-resolves `Defined` symbols that point into merge sections. It encodes the symbol value as:

```text
((pieceIdx + 1) << mergeValueShift) | intraPieceOffset
```

Later, `getParentOffset` can find the piece output offset in O(1), add the intra-piece offset, and compute the final address.

中文：

这一步对 `.rodata.pnu` 尤其重要。比如 `{2,3}` 的 symbol 原本在输入 section offset 12；tail merge 后，它的 output address 可能变成 `{1,2,3}` piece 的 output offset + 4。修正后的 symbol value 使 relocation、symbol table 输出和 `llvm-nm` 看到的地址都一致。

English:

This step is especially important for `.rodata.pnu`. For example, the `{2,3}` symbol may originally be at input-section offset 12; after tail merging, its output address may become the output offset of the `{1,2,3}` piece plus 4. The repaired symbol value keeps relocations, symbol-table output, and `llvm-nm` addresses consistent.

### 7.5 Error handling / 错误处理

中文：

当前 lld 明确拒绝以下 `.rodata.pnu` 输入：

- writable `.rodata.pnu`
- object symbol range 超出 section
- object symbol ranges 互相重叠
- `.rodata.pnu` 自身带 relocation

English:

Current lld explicitly rejects the following `.rodata.pnu` inputs:

- writable `.rodata.pnu`
- object-symbol range outside the section
- overlapping object-symbol ranges
- relocations inside `.rodata.pnu`

中文：

拒绝 relocation 的原因是：当前 PNU merging 是按原始字节内容做 hash 和 tail merge。relocation 会在链接末期改变 section 内容；如果在 relocation 还没应用时就按字节合并，可能把链接后内容不同的对象错误合并。普通 mergeable section 遇到 relocation 时可以降级为 non-mergeable；但 `.rodata.pnu` 的目的就是合并 PNU pieces，降级会悄悄失去语义目标，所以当前选择报错。

English:

Relocations are rejected because current PNU merging hashes and tail-merges raw bytes. Relocations mutate section contents late in linking; if objects are merged before applying relocations, the linker could incorrectly merge objects whose final contents differ. Ordinary mergeable sections may degrade to non-mergeable when relocations are present; `.rodata.pnu` exists specifically to merge PNU pieces, so silently degrading would lose the intended behavior. The current implementation reports an error instead.

## 8. Implementation Files / 实现文件

中文：

主要实现文件：

- `clang/lib/CodeGen/CGExpr.cpp`
  - 判断 initializer-list backing array 是否能静态发射。
- `clang/lib/CodeGen/CodeGenModule.h`
  - 新增或使用 `StaticInitListBackingArrayMap`。
- `clang/lib/CodeGen/CodeGenModule.cpp`
  - 创建 backing array global。
  - 为 ELF + no relocation 选择 `.rodata.pnu`。
  - PNU path 使用 `internal`，不使用 `unnamed_addr`。
- `llvm/lib/MC/ELFObjectWriter.cpp`
  - 保留 `.rodata.pnu` 中 object symbol relocation。
- `lld/ELF/InputFiles.cpp`
  - 把 `.rodata.pnu` 识别为特殊 `MergeInputSection`。
  - 拒绝 writable 和 relocation-bearing PNU section。
- `lld/ELF/InputSection.h`
  - 扩展 `MergeInputSection`，加入 PNU piece end offsets。
- `lld/ELF/InputSection.cpp`
  - 实现 `splitPnuPieces`。
  - 修改 `getData` 和 `getSectionPiece` 以支持 variable-sized PNU pieces。
- `lld/ELF/OutputSections.cpp`
  - 为 `.rodata.pnu` 选择 tail-merging synthetic section。
- `lld/ELF/SyntheticSections.cpp`
  - 复用 merge section finalization 和 symbol piece pre-resolution。

English:

Main implementation files:

- `clang/lib/CodeGen/CGExpr.cpp`
  - Decides whether an initializer-list backing array can be emitted statically.
- `clang/lib/CodeGen/CodeGenModule.h`
  - Adds or uses `StaticInitListBackingArrayMap`.
- `clang/lib/CodeGen/CodeGenModule.cpp`
  - Creates the backing-array global.
  - Selects `.rodata.pnu` for ELF + no relocation.
  - Uses `internal` and avoids `unnamed_addr` on the PNU path.
- `llvm/lib/MC/ELFObjectWriter.cpp`
  - Preserves object-symbol relocations in `.rodata.pnu`.
- `lld/ELF/InputFiles.cpp`
  - Recognizes `.rodata.pnu` as a special `MergeInputSection`.
  - Rejects writable and relocation-bearing PNU sections.
- `lld/ELF/InputSection.h`
  - Extends `MergeInputSection` with PNU piece end offsets.
- `lld/ELF/InputSection.cpp`
  - Implements `splitPnuPieces`.
  - Updates `getData` and `getSectionPiece` for variable-sized PNU pieces.
- `lld/ELF/OutputSections.cpp`
  - Selects a tail-merging synthetic section for `.rodata.pnu`.
- `lld/ELF/SyntheticSections.cpp`
  - Reuses merge-section finalization and symbol-piece pre-resolution.

## 9. Test Strategy / 测试策略

中文：

当前测试覆盖分成四层：

1. Clang IR / assembly tests
   - `clang/test/CodeGenCXX/p2752r3-initializer-list.cpp`
   - `clang/test/CodeGenCXX/cxx0x-initializer-stdinitializerlist.cpp`
   - 验证静态 backing array 被发射为 `.rodata.pnu`。
   - 验证 PNU globals 不再带 `unnamed_addr`。
   - 验证 assembly 中 `.rodata.pnu` 是 alloc-only，不是 `aM`。

2. Clang object-level regression
   - 同一个 `p2752r3-initializer-list.cpp` 通过 `llvm-readobj --sections` 检查 object section。
   - 目标是防止 PNU global 再次被 MC 放进 `SHF_MERGE` section。

3. MC relocation preservation
   - `llvm/test/MC/ELF/rodata-pnu-reloc.s`
   - 验证 `.rodata.pnu` local object symbol 的 relocation 不被改写成 section symbol。

4. lld merge and error-path tests
   - `lld/test/ELF/pnu-section.s`
   - `lld/test/ELF/pnu-section-errors.s`
   - 验证 exact merge、tail merge、local object symbol、PC-relative relocation use site、writable/oob/overlap/relocation 错误。

English:

The current test coverage has four layers:

1. Clang IR / assembly tests
   - `clang/test/CodeGenCXX/p2752r3-initializer-list.cpp`
   - `clang/test/CodeGenCXX/cxx0x-initializer-stdinitializerlist.cpp`
   - Verify that static backing arrays are emitted into `.rodata.pnu`.
   - Verify that PNU globals no longer have `unnamed_addr`.
   - Verify that assembly uses alloc-only `.rodata.pnu`, not `aM`.

2. Clang object-level regression
   - The same `p2752r3-initializer-list.cpp` checks object sections using `llvm-readobj --sections`.
   - The goal is to prevent PNU globals from being emitted into `SHF_MERGE` sections again.

3. MC relocation preservation
   - `llvm/test/MC/ELF/rodata-pnu-reloc.s`
   - Verifies that relocations against local object symbols in `.rodata.pnu` are not rewritten to section symbols.

4. lld merge and error-path tests
   - `lld/test/ELF/pnu-section.s`
   - `lld/test/ELF/pnu-section-errors.s`
   - Verify exact merging, tail merging, local object symbols, PC-relative relocation use sites, and writable/oob/overlap/relocation errors.

中文：

本地 smoke test 还验证了一个端到端场景：

```text
TU A: {1,2,3}, {2,3}
TU B: {1,2,3}
```

链接后 `llvm-nm` 显示：

```text
base      r .init.list
base      r .init.list
base + 4  r .init.list.1
```

`llvm-readelf -x .rodata` 只显示 12 字节 `{1,2,3}`。

English:

A local smoke test also verifies an end-to-end scenario:

```text
TU A: {1,2,3}, {2,3}
TU B: {1,2,3}
```

After linking, `llvm-nm` shows:

```text
base      r .init.list
base      r .init.list
base + 4  r .init.list.1
```

`llvm-readelf -x .rodata` shows only the 12 bytes for `{1,2,3}`.

## 10. Build and Verification Commands / 构建与验证命令

中文：

当前分支使用的 CMake project 集合：

```bash
cmake -S llvm -B ../rel -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld"
```

核心构建命令：

```bash
ninja -C ../rel clang lld llvm-mc llvm-readobj llvm-readelf llvm-nm
```

English:

The current branch uses this CMake project set:

```bash
cmake -S llvm -B ../rel -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld"
```

Core build command:

```bash
ninja -C ../rel clang lld llvm-mc llvm-readobj llvm-readelf llvm-nm
```

中文：

相关 lit 覆盖集：

```bash
../rel/bin/llvm-lit -sv \
  llvm/test/MC/ELF/rodata-pnu-reloc.s \
  clang/test/CodeGenCXX/p2752r3-initializer-list.cpp \
  clang/test/CodeGenCXX/cxx0x-initializer-stdinitializerlist.cpp \
  clang/test/CodeGenCXX/cxx0x-initializer-stdinitializerlist-startend.cpp \
  clang/test/CodeGenCXX/cxx0x-initializer-stdinitializerlist-pr12086.cpp \
  clang/test/CodeGenCXX/PR24289.cpp \
  clang/test/AST/ByteCode/initializer_list.cpp \
  clang/test/CXX/drs/cwg27xx.cpp \
  lld/test/ELF/pnu-section.s \
  lld/test/ELF/pnu-section-errors.s \
  lld/test/ELF/merge-align2.s \
  lld/test/ELF/merge-string-align.s \
  lld/test/ELF/merge-string-align2.s \
  lld/test/ELF/merge-reloc.s \
  lld/test/ELF/gc-merge-local-sym.s
```

English:

Relevant lit coverage set:

```bash
../rel/bin/llvm-lit -sv \
  llvm/test/MC/ELF/rodata-pnu-reloc.s \
  clang/test/CodeGenCXX/p2752r3-initializer-list.cpp \
  clang/test/CodeGenCXX/cxx0x-initializer-stdinitializerlist.cpp \
  clang/test/CodeGenCXX/cxx0x-initializer-stdinitializerlist-startend.cpp \
  clang/test/CodeGenCXX/cxx0x-initializer-stdinitializerlist-pr12086.cpp \
  clang/test/CodeGenCXX/PR24289.cpp \
  clang/test/AST/ByteCode/initializer_list.cpp \
  clang/test/CXX/drs/cwg27xx.cpp \
  lld/test/ELF/pnu-section.s \
  lld/test/ELF/pnu-section-errors.s \
  lld/test/ELF/merge-align2.s \
  lld/test/ELF/merge-string-align.s \
  lld/test/ELF/merge-string-align2.s \
  lld/test/ELF/merge-reloc.s \
  lld/test/ELF/gc-merge-local-sym.s
```

## 11. Current Limitations / 当前限制

中文：

已知限制：

- 只支持 ELF + lld 的最终链接。`ld -r` / relocatable output 不触发 `.rodata.pnu` merging。
- `.rodata.pnu` 不支持 relocation-bearing contents。
- `.rodata.pnu` 必须只读，不能 writable。
- 当前 piece metadata 依赖 `.symtab` 中的 object symbols，而不是独立 `.pnu_symtab`。
- 当前实现拒绝重叠的输入 symbol ranges；重叠只能作为 lld 输出合并结果产生。
- PNU synthetic section 当前要求兼容的 alignment grouping；不同 alignment 的 PNU input sections 不一定处于同一个 merge domain。
- 非 Clang producer 如果没有发射 object symbols，fallback 会把整个 `.rodata.pnu` 当一个 piece，合并粒度较粗。
- 这个机制不适用于必须由语言保证“同一对象”的实体，例如 inline variables 或可能的 `define_static_array` 强语义场景。

English:

Known limitations:

- Supports final ELF links with lld only. `ld -r` / relocatable output does not perform `.rodata.pnu` merging.
- `.rodata.pnu` does not support relocation-bearing contents.
- `.rodata.pnu` must be read-only, not writable.
- Piece metadata currently depends on object symbols in `.symtab`, not on a separate `.pnu_symtab`.
- The implementation rejects overlapping input symbol ranges; overlap is only produced as an lld output-merge result.
- The current PNU synthetic-section grouping requires compatible alignment; PNU input sections with different alignments may not share the same merge domain.
- A non-Clang producer that emits no object symbols falls back to treating the whole `.rodata.pnu` as one piece, producing coarser merging.
- This mechanism is not suitable for entities whose identity must be guaranteed by the language, such as inline variables or possible strong-semantics uses of `define_static_array`.

## 12. Future Work / 后续工作

中文：

合理的后续方向：

1. 引入显式 `.pnu_symtab` 或类似 metadata section
   - 可以减少对完整 symbol table 的依赖。
   - 可以把 piece metadata 设计得更紧凑。
   - 需要定义 object format contract 和兼容策略。

2. 研究 relocation-bearing PNU
   - 需要在 relocation 应用后比较内容，或者把 relocation target/addend 纳入等价性判断。
   - 复杂度明显高于当前 raw-byte merging。

3. 改善 alignment 策略
   - 评估不同 alignment 的 PNU pieces 是否可以在同一个 tail-merge builder 中安全合并。
   - 需要保证输出地址满足每个 symbol 的 alignment。

4. 加强端到端测试
   - 当前已有本地 smoke test 和 object-level regression。
   - 可以考虑增加 cross-project test，但要避免让 lld 单元测试依赖 Clang 前端。

5. 扩展到其他 PNU 来源
   - 只有在语言语义允许 false negatives 且禁止 false positives 的场景才适合。
   - `define_static_array` 如果要求强制合并相同对象，则需要不同机制，可能更接近 COMDAT/name-based canonicalization 或未来标准/ABI 定义。

English:

Reasonable future directions:

1. Add an explicit `.pnu_symtab` or similar metadata section
   - Could reduce dependence on the full symbol table.
   - Could make piece metadata more compact.
   - Requires defining an object-format contract and compatibility strategy.

2. Investigate relocation-bearing PNU
   - Would require comparing contents after relocation application, or including relocation target/addend information in equivalence.
   - This is significantly more complex than current raw-byte merging.

3. Improve alignment handling
   - Evaluate whether PNU pieces with different alignments can safely share one tail-merge builder.
   - Must guarantee that every output symbol address satisfies its required alignment.

4. Strengthen end-to-end tests
   - The branch already has a local smoke test and object-level regression.
   - A cross-project test may be useful, but lld unit tests should avoid depending on the Clang frontend.

5. Extend to other PNU sources
   - This is suitable only where language semantics allow false negatives and forbid false positives.
   - If `define_static_array` requires mandatory merging of identical objects, it needs a different mechanism, likely closer to COMDAT/name-based canonicalization or a future standard/ABI definition.

## 13. Glossary / 术语表

中文：

- PNU：Potentially Non-Unique Object，允许实现合并或重叠的对象。
- backing array：`std::initializer_list` 元素实际存放的数组。
- exact merge：两个对象的完整字节内容相同，输出中只保留一个。
- tail merge：一个对象的完整字节内容等于另一个对象的尾部，输出 symbol 指向较大对象内部。
- `SHF_MERGE`：ELF section flag，表示 linker 可以合并 section 中的重复元素。
- COMDAT / `SHF_GROUP`：按 group signature / symbol name 去重的一类 section 机制。
- `unnamed_addr`：LLVM IR 属性，表示地址身份不重要，后端可更积极地合并常量。

English:

- PNU: Potentially Non-Unique Object, an object that the implementation may merge or overlap.
- backing array: the actual array that stores the elements of a `std::initializer_list`.
- exact merge: two objects have identical full byte contents, so the output keeps only one.
- tail merge: one object's full byte contents equal the tail of another object, so the output symbol points inside the larger object.
- `SHF_MERGE`: an ELF section flag indicating that the linker may merge duplicate elements in the section.
- COMDAT / `SHF_GROUP`: a section mechanism that deduplicates by group signature / symbol name.
- `unnamed_addr`: an LLVM IR attribute indicating that address identity is not significant, enabling more aggressive constant merging by the backend.

