# Operation Definition Specification (ODS)

In addition to specializing the `mlir::Op` C++ template, MLIR also supports
defining operations and data types in a table-driven manner. This is achieved
via [TableGen][TableGen], which is both a generic language and its tooling to
maintain records of domain-specific information. Facts regarding an operation
are specified concisely into a TableGen record, which will be expanded into an
equivalent `mlir::Op` C++ template specialization at compiler build time.

This manual explains in detail all the available mechanisms for defining
operations in such a table-driven manner. It aims to be a specification instead
of a tutorial. Please refer to
[Quickstart tutorial to adding MLIR graph rewrite](../Tutorials/QuickstartRewrites.md)
for the latter.

In addition to detailing each mechanism, this manual also tries to capture best
practices. They are rendered as quoted bullet points.

[TOC]

## Motivation

MLIR allows pluggable dialects, and dialects contain, among others, a list of
operations. This open and extensible ecosystem leads to the "stringly" type IR
problem, e.g., repetitive string comparisons during optimization and analysis
passes, unintuitive accessor methods (e.g., generic/error prone `getOperand(3)`
vs self-documenting `getStride()`) with more generic return types, verbose and
generic constructors without default arguments, verbose textual IR dumps, and so
on. Furthermore, operation verification is:

1.  best case: a central string-to-verification-function map,
1.  middle case: duplication of verification across the code base, or
1.  worst case: no verification functions.

The fix is to support defining ops in a table-driven manner. Then for each
dialect, we can have a central place that contains everything you need to know
about each op, including its constraints, custom assembly form, etc. This
description is also used to generate helper functions and classes to allow
building, verification, parsing, printing, analysis, and many more.

## Benefits

Compared to the C++ template, this table-driven approach has several benefits
including but not limited to:

*   **Single source of truth**: We strive to encode all facts regarding an
    operation into the record, so that readers don't need to jump among code
    snippets to fully understand an operation.
*   **Removing boilerplate**: We can automatically generate
    operand/attribute/result getter methods, operation build methods, operation
    verify methods, and many more utilities from the record. This greatly
    reduces the boilerplate needed for defining a new op.
*   **Facilitating auto-generation**: The usage of these operation information
    records are by no means limited to op definition itself. We can use them to
    drive the auto-generation of many other components, like computation graph
    serialization.

## TableGen Syntax

We use TableGen as the language for specifying operation information. TableGen
itself just provides syntax for writing records; the syntax and constructs
allowed in a TableGen file (typically with the filename suffix `.td`) can be found
[here][TableGenProgRef].

*   TableGen `class` is similar to C++ class; it can be templated and
    subclassed.
*   TableGen `def` is similar to C++ object; it can be declared by specializing
    a TableGen `class` (e.g., `def MyDef : MyClass<...>;`) or completely
    independently (e.g., `def MyDef;`). It cannot be further templated or
    subclassed.
*   TableGen `dag` is a dedicated type for directed acyclic graph of elements. A
    `dag` has one operator and zero or more arguments. Its syntax is `(operator
    arg0, arg1, argN)`. The operator can be any TableGen `def`; an argument can
    be anything, including `dag` itself. We can have names attached to both the
    operator and the arguments like `(MyOp:$op_name MyArg:$arg_name)`.

Please see the [language reference][TableGenProgRef] to learn about all the
types and expressions supported by TableGen.

## Operation Definition

MLIR defines several common constructs to help operation definition and provide
their semantics via a special [TableGen backend][TableGenBackend]:
[`OpDefinitionsGen`][OpDefinitionsGen]. These constructs are defined in
[`OpBase.td`][OpBase]. The main ones are:

*   The `Op` class: It is the main construct for defining operations. All facts
    regarding the operation are specified when specializing this class, with the
    help of the following constructs.
*   The `Dialect` class: Operations belonging to one logical group are placed in
    the same dialect. The `Dialect` class contains dialect-level information.
*   The `Trait` class hierarchy: They are used to specify special properties
    and constraints of the operation, including whether the operation has side
    effect or whether its output has the same shape as the input.
*   The `ins`/`outs` marker: These are two special markers builtin to the
    `OpDefinitionsGen` backend. They lead to the definitions of operands/attributes
    and results respectively.
*   The `TypeConstraint` class hierarchy: They are used to specify the
    constraints over operands or results. A notable subclass hierarchy is
    `Type`, which stands for constraints for common C++ types.
*   The `AttrConstraint` class hierarchy: They are used to specify the
    constraints over attributes. A notable subclass hierarchy is `Attr`, which
    stands for constraints for attributes whose values are of common types.
*   The `Property` class hierarchy: They are used to specify non-attribute-backed
    properties that are inherent to operations. These properties can have
    constraints imposed on them using the `predicate` field or the
    `ConfinedProp` class. The `PropConstraint` superclass of `Property` is used
    to describe constraints on properties in rewrite patterns.

An operation is defined by specializing the `Op` class with concrete contents
for all the fields it requires. For example, `tf.AvgPool` is defined as

```tablegen
def TF_AvgPoolOp : TF_Op<"AvgPool", [NoMemoryEffect]> {
  let summary = "Performs average pooling on the input.";

  let description = [{
Each entry in `output` is the mean of the corresponding size `ksize`
window in `value`.
  }];

  let arguments = (ins
    TF_FpTensor:$value,

    ConfinedAttr<I64ArrayAttr, [ArrayMinCount<4>]>:$ksize,
    ConfinedAttr<I64ArrayAttr, [ArrayMinCount<4>]>:$strides,
    TF_AnyStrAttrOf<["SAME", "VALID"]>:$padding,
    DefaultValuedAttr<TF_ConvertDataFormatAttr, "NHWC">:$data_format
  );

  let results = (outs
    TF_FpTensor:$output
  );

  TF_DerivedOperandTypeAttr T = TF_DerivedOperandTypeAttr<0>;
}
```

In the following we describe all the fields needed. Please see the definition of
the `Op` class for the complete list of fields supported.

### Operation name

The operation name is a unique identifier for the operation within MLIR, e.g.,
`tf.Add` for addition operation in the TensorFlow dialect. This is the
equivalent of the mnemonic in assembly language. It is used for parsing and
printing in the textual format. It is also used for pattern matching in graph
rewrites.

The full operation name is composed of the dialect name and the op name, with
the former provided via the dialect and the latter provided as the second
template parameter to the `Op` class.

### Operation documentation

This includes both a one-line `summary` and a longer human-readable
`description`. They will be used to drive automatic generation of dialect
documentation. They need to be provided in the operation's definition body:

```tablegen
let summary = "...";

let description = [{
...
}];
```

`description` should be written in Markdown syntax.

Placing the documentation at the beginning is recommended since it helps in
understanding the operation.

> *   Place documentation at the beginning of the operation definition.
> *   The summary should be short and concise. It should be a one-liner
>     starting with a capital letter and without trailing punctuation.
>     Put expanded explanation in the description.

### Operation arguments

There are three kinds of arguments: operands, attributes, and properties.
Operands are runtime values produced by other ops; while attributes and properties
are compile-time known constant values, including two categories:

1.  Natural attributes: these attributes affect the behavior of the operations
    (e.g., padding for convolution);
1.  Derived attributes: these attributes are not needed to define the operation
    but are instead derived from information of the operation. E.g., the output
    shape of type. This is mostly used for convenience interface generation or
    interaction with other frameworks/translation.

    All derived attributes should be materializable as an Attribute. That is,
    even though they are not materialized, it should be possible to store as an
    attribute.

Properties are similar to attributes, except that they are not stored within
the MLIR context but are stored inline with the operation.

Operands, attributes, and properties are specified inside the `dag`-typed
`arguments`, led by `ins`:

```tablegen
let arguments = (ins
  <type-constraint>:$<operand-name>,
  ...
  <attr-constraint>:$<attr-name>,
  ...
  <property>:$<property-name>,
);
```

Here `<type-constraint>` is a TableGen `def` from the `TypeConstraint` class
hierarchy. Similarly, `<attr-constraint>` is a TableGen `def` from the
`AttrConstraint` class hierarchy and `<property>` is a subclass
of `Property` (constraints can be imposed onto it using its `predicate` field
or the `ConfinedProp` subclass).

There are no requirements on the relative order of operands and attributes; they
can mix freely. The relative order of operands themselves matters. From each
named argument a named getter will be generated that returns the argument with
the return type (in the case of attributes the return type will be constructed
from the storage type, while for operands it will be `Value`). Each attribute's
raw value (e.g., as stored) can also be accessed via generated `<name>Attr`
getters for use in transformation passes where the more user-friendly return
type is less suitable.

All the arguments should be named to:
- provide documentation,
- drive auto-generation of getter methods, and
- provide a handle to reference for other places like constraints.

#### Variadic operands

To declare a variadic operand, wrap the `TypeConstraint` for the operand with
`Variadic<...>`.

Normally operations have no variadic operands or just one variadic operand. For
the latter case, it is easy to deduce which dynamic operands are for the static
variadic operand definition. However, if an operation has more than one variable
length operands (either optional or variadic), it would be impossible to
attribute dynamic operands to the corresponding static variadic operand
definitions without further information from the operation. Therefore, either
the `SameVariadicOperandSize` or `AttrSizedOperandSegments` trait is needed to
indicate that all variable length operands have the same number of dynamic
values.

#### VariadicOfVariadic operands

To declare a variadic operand that has a variadic number of sub-ranges, wrap the
`TypeConstraint` for the operand with `VariadicOfVariadic<...,
"<segment-attribute-name>">`.

The second field of the `VariadicOfVariadic` is the name of a `DenseI32ArrayAttr`
argument that contains the sizes of the variadic sub-ranges. This attribute will
be used when determining the size of sub-ranges, or when updating the size of
sub-ranges.

#### Optional operands

To declare an optional operand, wrap the `TypeConstraint` for the operand with
`Optional<...>`.

Normally operations have no optional operands or just one optional operand. For
the latter case, it is easy to deduce which dynamic operands are for the static
operand definition. However, if an operation has more than one variable length
operands (either optional or variadic), it would be impossible to attribute
dynamic operands to the corresponding static variadic operand definitions
without further information from the operation. Therefore, either the
`SameVariadicOperandSize` or `AttrSizedOperandSegments` trait is needed to
indicate that all variable length operands have the same number of dynamic
values.

#### Optional attributes

To declare an optional attribute, wrap the `AttrConstraint` for the attribute
with `OptionalAttr<...>`.

#### Attributes with default values

To declare an attribute with a default value, wrap the `AttrConstraint` for the
attribute with `DefaultValuedAttr<..., "...">`.

The second parameter to `DefaultValuedAttr` should be a string containing the
C++ default value. For example, a float default value should be specified as
like `"0.5f"`, and an integer array default value should be specified as like
`"{1, 2, 3}"`.

The generated operation printing function will not print default-valued
attributes when the attribute value is equal to the default.

#### Confining attributes

`ConfinedAttr` is provided as a general mechanism to help modelling further
constraints on attributes beyond the ones brought by value types. You can use
`ConfinedAttr` to compose complex constraints out of more primitive ones. For
example, a 32-bit integer attribute whose minimum value must be 10 can be
expressed as `ConfinedAttr<I32Attr, [IntMinValue<10>]>`.

Right now, the following primitive constraints are supported:

*   `IntMinValue<N>`: Specifying an integer attribute to be greater than or
    equal to `N`
*   `IntMaxValue<N>`: Specifying an integer attribute to be less than or equal
    to `N`
*   `IntNEQValue<N>`: Specifying an integer attribute to be not equal
    to `N`
*   `IntPositive`: Specifying an integer attribute whose value is positive
*   `IntNonNegative`: Specifying an integer attribute whose value is
    non-negative
*   `IntPowerOf2`: Specifying an integer attribute whose value is a power of
    two > 0
*   `ArrayMinCount<N>`: Specifying an array attribute to have at least `N`
    elements
*   `ArrayMaxCount<N>`: Specifying an array attribute to have at most `N`
    elements
*   `ArrayCount<N>`: Specifying an array attribute to have exactly `N`
    elements
*   `DenseArrayCount<N>`: Specifying a dense array attribute to have
    exactly `N` elements
*   `DenseArrayStrictlyPositive<arrayType>`: Specifying a dense array attribute
    of type `arrayType` to have all positive elements
*   `DenseArrayStrictlyNonNegative<arrayType>`: Specifying a dense array attribute
    of type `arrayType` to have all non-negative elements
*   `DenseArraySorted<arrayType>`: Specifying a dense array attribute
    of type `arrayType` to have elements in non-decreasing order
*   `DenseArrayStrictlySorted<arrayType>`: Specifying a dense array attribute
    of type `arrayType` to have elements in increasing order
*   `IntArrayNthElemEq<I, N>`: Specifying an integer array attribute's `I`-th
    element to be equal to `N`
*   `IntArrayNthElemMinValue<I, N>`: Specifying an integer array attribute's
    `I`-th element to be greater than or equal to `N`
*   `IntArrayNthElemMaxValue<I, N>`: Specifying an integer array attribute's
    `I`-th element to be less than or equal to `N`
*   `IntArrayNthElemInRange<I, M, N>`: Specifying an integer array attribute's
    `I`-th element to be greater than or equal to `M` and less than or equal to `N`
*   `IsNullAttr`: Specifying an optional attribute which must be empty

TODO: Design and implement more primitive constraints

#### Optional and default-valued properties

To declare a property with a default value, use `DefaultValuedProp<..., "...">`.
If the property's storage data type is different from its interface type,
for example, in the case of array properties (which are stored as `SmallVector`s
but use `ArrayRef` as an interface type), add the storage-type equivalent
of the default value as the third argument.

To declare an optional property, use `OptionalProp<...>`.
This wraps the underlying property in an `std::optional` and gives it a
default value of `std::nullopt`.

#### Combining constraints

`AllAttrOf` is provided to allow combination of multiple constraints which
must all hold.

For example:
```tablegen
def OpAllAttrConstraint1 : TEST_Op<"all_attr_constraint_of1"> {
  let arguments = (ins I64ArrayAttr:$attr);
  let results = (outs I32);
}
def OpAllAttrConstraint2 : TEST_Op<"all_attr_constraint_of2"> {
  let arguments = (ins I64ArrayAttr:$attr);
  let results = (outs I32);
}
def Constraint0 : AttrConstraint<
    CPred<"::llvm::cast<::mlir::IntegerAttr>(::llvm::cast<ArrayAttr>($_self)[0]).getInt() == 0">,
    "[0] == 0">;
def Constraint1 : AttrConstraint<
    CPred<"::llvm::cast<::mlir::IntegerAttr>(::llvm::cast<ArrayAttr>($_self)[1]).getInt() == 1">,
    "[1] == 1">;
def : Pat<(OpAllAttrConstraint1
            AllAttrOf<[Constraint0, Constraint1]>:$attr),
          (OpAllAttrConstraint2 $attr)>;
```

### Operation regions

The regions of an operation are specified inside of the `dag`-typed `regions`,
led by `region`:

```tablegen
let regions = (region
  <region-constraint>:$<region-name>,
  ...
);
```

#### Variadic regions

Similar to the `Variadic` class used for variadic operands and results,
`VariadicRegion<...>` can be used for regions. Variadic regions can currently
only be specified as the last region in the regions list.

### Operation results

Similar to operands, results are specified inside the `dag`-typed `results`, led
by `outs`:

```tablegen
let results = (outs
  <type-constraint>:$<result-name>,
  ...
);
```

#### Variadic results

Similar to variadic operands, `Variadic<...>` can also be used for results. And
similarly, `SameVariadicResultSize` for multiple variadic results in the same
operation.

### Operation successors

For terminator operations, the successors are specified inside of the
`dag`-typed `successors`, led by `successor`:

```tablegen
let successors = (successor
  <successor-constraint>:$<successor-name>,
  ...
);
```

#### Variadic successors

Similar to the `Variadic` class used for variadic operands and results,
`VariadicSuccessor<...>` can be used for successors. Variadic successors can
currently only be specified as the last successor in the successor list.

### Operation traits and constraints

Traits are operation properties that affect syntax or semantics. MLIR C++ models
various traits in the `mlir::OpTrait` namespace.

Both operation traits, [interfaces](../Interfaces.md/#utilizing-the-ods-framework),
and constraints involving multiple operands/attributes/results are provided as
the third template parameter to the `Op` class. They should be deriving from
the `Trait` class. See [Constraints](#constraints) for more information.

### Builder methods

For each operation, there are a few builders automatically generated based on
the arguments and returns types. For example, given the following op definition:

```tablegen
def MyOp : ... {
  let arguments = (ins
    I32:$i32_operand,
    F32:$f32_operand,
    ...,

    I32Attr:$i32_attr,
    F32Attr:$f32_attr,
    ...
    I32Prop:$i32_prop,
    ...
  );

  let results = (outs
    I32:$i32_result,
    F32:$f32_result,
    ...
  );
}
```

The following builders are generated:

```c++
// All result-types/operands/properties/discardable attributes have one
// aggregate parameter. `Properties` is the properties structure of
// `MyOp`.
static void build(OpBuilder &odsBuilder, OperationState &odsState,
                  TypeRange resultTypes,
                  ValueRange operands,
                  Properties properties,
                  ArrayRef<NamedAttribute> discardableAttributes = {});

// All result-types/operands/attributes have one aggregate parameter.
// Inherent properties and discardable attributes are mixed together in the
//  `attributes` dictionary.
static void build(OpBuilder &odsBuilder, OperationState &odsState,
                  TypeRange resultTypes,
                  ValueRange operands,
                  ArrayRef<NamedAttribute> attributes);

// Each result-type/operand/attribute has a separate parameter. The parameters
// for attributes are of mlir::Attribute types.
static void build(OpBuilder &odsBuilder, OperationState &odsState,
                  Type i32_result, Type f32_result, ...,
                  Value i32_operand, Value f32_operand, ...,
                  IntegerAttr i32_attr, FloatAttr f32_attr, ...,
                  int32_t i32_prop);

// Each result-type/operand/attribute has a separate parameter. The parameters
// for attributes are raw values unwrapped with mlir::Attribute instances.
// (Note that this builder will not always be generated. See the following
// explanation for more details.)
static void build(OpBuilder &odsBuilder, OperationState &odsState,
                  Type i32_result, Type f32_result, ...,
                  Value i32_operand, Value f32_operand, ...,
                  APInt i32_attr, StringRef f32_attr, ...,
                  int32_t i32_prop, ...);

// Each operand/attribute has a separate parameter but result type is aggregate.
static void build(OpBuilder &odsBuilder, OperationState &odsState,
                  TypeRange resultTypes,
                  Value i32_operand, Value f32_operand, ...,
                  IntegerAttr i32_attr, FloatAttr f32_attr, ...,
                  int32_t i32_prop, ...);

// All operands/attributes have aggregate parameters.
// Generated if return type can be inferred.
static void build(OpBuilder &odsBuilder, OperationState &odsState,
                  ValueRange operands,
                  Properties properties,
                  ArrayRef<NamedAttribute> discardableAttributes);

// All operands/attributes have aggregate parameters.
// Generated if return type can be inferred. Uses the legacy merged attribute
// dictionary.
static void build(OpBuilder &odsBuilder, OperationState &odsState,
                  ValueRange operands, ArrayRef<NamedAttribute> attributes);

// (And manually specified builders depending on the specific op.)
```

The first two forms provide basic uniformity so that we can create ops using
the same form regardless of the exact op. This is particularly useful for
implementing declarative pattern rewrites.

The third and fourth forms are good for use in manually written code, given that
they provide better guarantee via signatures.

The fourth form will be generated if any of the op's attribute has different
`Attr.returnType` from `Attr.storageType` and we know how to build an attribute
from an unwrapped value (i.e., `Attr.constBuilderCall` is defined.)
Additionally, for the third form, if an attribute appearing later in the
`arguments` list has a default value, the default value will be supplied in the
declaration. This works for `BoolAttr`, `StrAttr`, `EnumAttr` for now and the
list can grow in the future. So if possible, the default-valued attribute should be
placed at the end of the `arguments` list to leverage this feature. (This
behavior is essentially due to C++ function parameter default value placement
restrictions.) Otherwise, the builder of the third form will still be generated
but default values for the attributes not at the end of the `arguments` list
will not be supplied in the builder's signature.

ODS will generate a builder that doesn't require the return type specified if

*   Op implements InferTypeOpInterface interface;
*   All return types are either buildable types or are the same as a given
    operand (e.g., `AllTypesMatch` constraint between operand and result);

And there may potentially exist other builders depending on the specific op;
please refer to the
[generated C++ file](#run-mlir-tblgen-to-see-the-generated-content) for the
complete list.

#### Custom builder methods

However, if the above cases cannot satisfy all needs, you can define additional
convenience build methods in the `builders` field as follows.

```tablegen
def MyOp : Op<"my_op", []> {
  let arguments = (ins F32Attr:$attr);

  let builders = [
    OpBuilder<(ins "float":$val)>
  ];
}
```

The `builders` field is a list of custom builders that are added to the Op
class. In this example, we provide a convenience builder that takes a floating
point value instead of an attribute. The `ins` prefix is common to many function
declarations in ODS, which use a TableGen [`dag`](#tablegen-syntax). What
follows is a comma-separated list of types (quoted string) and names prefixed
with the `$` sign. This will generate the declaration of a builder method that
looks like:

```c++
class MyOp : /*...*/ {
  /*...*/
  static void build(::mlir::OpBuilder &builder, ::mlir::OperationState &state,
                    float val);
};
```

Note that the method has two additional leading arguments. These arguments are
useful to construct the operation. In particular, the method must populate
`state` with attributes, operands, regions and result types of the operation to
be constructed. `builder` can be used to construct any IR objects that belong to
the Op, such as types or nested operations. Since the type and name are
generated as is in the C++ code, they should be valid C++ constructs for a type
(in the namespace of the Op) and an identifier (e.g., `class` is not a valid
identifier).

Implementations of the builder can be provided directly in ODS, using TableGen
code block as follows.

```tablegen
def MyOp : Op<"my_op", []> {
  let arguments = (ins F32Attr:$attr);

  let builders = [
    OpBuilder<(ins "float":$val), [{
      $_state.addAttribute("attr", $_builder.getF32FloatAttr(val));
    }]>
  ];
}
```

The equivalents of `builder` and `state` arguments are available as `$_builder`
and `$_state` special variables. The named arguments listed in the `ins` part
are available directly, e.g. `val`. The body of the builder will be generated by
substituting special variables and should otherwise be valid C++. While there is
no limitation on the code size, we encourage one to define only short builders
inline in ODS and put definitions of longer builders in C++ files.

Finally, if some arguments need a default value, they can be defined using
`CArg` to wrap the type and this value as follows.

```tablegen
def MyOp : Op<"my_op", []> {
  let arguments = (ins F32Attr:$attr);

  let builders = [
    OpBuilder<(ins CArg<"float", "0.5f">:$val), [{
      $_state.addAttribute("attr", $_builder.getF32FloatAttr(val));
    }]>
  ];
}
```

The generated code will use default value in the declaration, but not in the
definition, as required by C++.

```c++
/// Header file.
class MyOp : /*...*/ {
  /*...*/
  static void build(::mlir::OpBuilder &builder, ::mlir::OperationState &state,
                    float val = 0.5f);
};

/// Source file.
MyOp::build(::mlir::OpBuilder &builder, ::mlir::OperationState &state,
            float val) {
  state.addAttribute("attr", builder.getF32FloatAttr(val));
}
```

### Custom parser and printer methods

Functions to parse and print the operation's custom assembly form.

### Custom verifier code

Verification code will be automatically generated for
[constraints](#constraints) specified on various entities of the op. To perform
_additional_ verification, you can use

```tablegen
let hasVerifier = 1;
let hasRegionVerifier = 1;
```

This will generate `LogicalResult verify()`/`LogicalResult verifyRegions()`
method declarations on the op class that can be defined with any additional
verification constraints. For verificaiton which needs to access the nested
operations, you should use `hasRegionVerifier` to ensure that it won't access
any ill-formed operation. Except that, The other verifications can be
implemented with `hasVerifier`. Check the next section for the execution order
of these verification methods.

#### Verification Ordering

The verification of an operation involves several steps,

1. StructuralOpTrait will be verified first, they can be run independently.
2. `verifyInvariants` which is constructed by ODS, it verifies the type,
   attributes, .etc.
3. Other Traits/Interfaces that have marked their verifier as `verifyTrait` or
   `verifyWithRegions=0`.
4. Custom verifier which is defined in the op and has been marked `hasVerifier=1`

If an operation has regions, then it may have the second phase,

1. Traits/Interfaces that have marked their verifier as `verifyRegionTrait` or
   `verifyWithRegions=1`. This implies the verifier needs to access the
   operations in its regions.
2. Custom verifier which is defined in the op and has been marked
   `hasRegionVerifier=1`

Note that the second phase will be run after the operations in the region are
verified. Verifiers further down the order can rely on certain invariants being
verified by a previous verifier and do not need to re-verify them.

#### Emitting diagnostics in custom verifiers

Custom verifiers should avoid printing operations using custom operation
printers, because they require the printed operation (and sometimes its parent
operation) to be verified first. In particular, when emitting diagnostics,
custom verifiers should use the `Error` severity level, which prints operations
in generic form by default, and avoid using lower severity levels (`Note`,
`Remark`, `Warning`).

### Declarative Assembly Format

The custom assembly form of the operation may be specified in a declarative
string that matches the operations operands, attributes, etc. With the ability
to express additional information that needs to be parsed to build the
operation:

```tablegen
def CallOp : Std_Op<"call", ...> {
  let arguments = (ins FlatSymbolRefAttr:$callee, Variadic<AnyType>:$args);
  let results = (outs Variadic<AnyType>);

  let assemblyFormat = [{
    $callee `(` $args `)` attr-dict `:` functional-type($args, results)
  }];
}
```

The format is comprised of three components:

#### Directives

A directive is a type of builtin function, with an optional set of arguments.
The available directives are as follows:

*   `attr-dict`

    -   Represents the attribute dictionary of the operation.
    -   Any inherent attributes that are not used elsewhere in the format are
        printed as part of the attribute dictionary unless a `prop-dict` is
        present.
    -   Discardable attributes are always part of the `attr-dict`.

*   `attr-dict-with-keyword`

    -   Represents the attribute dictionary of the operation, but prefixes the
        dictionary with an `attributes` keyword.

*   `prop-dict`

    -   Represents the properties of the operation converted to a dictionary.
    -   Any property or inherent attribute that are not used elsewhere in the
        format are parsed and printed as part of this dictionary.
    -   If present, the `attr-dict` will not contain any inherent attributes.

*   `custom < UserDirective > ( Params )`

    -   Represents a custom directive implemented by the user in C++.
    -   See the [Custom Directives](#custom-directives) section below for more
        details.

*   `functional-type ( inputs , outputs )`

    -   Formats the `inputs` and `outputs` arguments as a
        [function type](../Dialects/Builtin.md/#functiontype).
    -   The constraints on `inputs` and `outputs` are the same as the `input` of
        the `type` directive.

*   ``oilist ( `keyword` elements | `otherKeyword` elements ...)``

    -   Represents an optional order-independent list of clauses. Each clause
        has a keyword and corresponding assembly format.
    -   Each clause can appear 0 or 1 time (in any order).
    -   Only literals, types and variables can be used within an oilist element.
    -   All the variables must be optional or variadic.

*   `operands`

    -   Represents all of the operands of an operation.

*   `ref ( input )`

    -   Represents a reference to a variable or directive, that must have
        already been resolved, that may be used as a parameter to a `custom`
        directive.
    -   Used to pass previously parsed entities to custom directives.
    -   The input may be any directive or variable, aside from `functional-type`
        and `custom`.

*   `regions`

    -   Represents all of the regions of an operation.

*   `results`

    -   Represents all of the results of an operation.

*   `successors`

    -   Represents all of the successors of an operation.

*   `type ( input )`

    -   Represents the type of the given input.
    -   `input` must be either an operand or result [variable](#variables), the
        `operands` directive, or the `results` directive.

*   `qualified ( type_or_attribute )`

    -   Wraps a `type` directive or an attribute parameter.
    -   Used to force printing the type or attribute prefixed with its dialect
        and mnemonic. For example the `vector.multi_reduction` operation has a
        `kind` attribute ; by default the declarative assembly will print:
        `vector.multi_reduction <minf>, ...` but using `qualified($kind)` in the
        declarative assembly format will print it instead as:
        `vector.multi_reduction #vector.kind<minf>, ...`.

#### Literals

A literal is either a keyword or punctuation surrounded by \`\`.

The following are the set of valid punctuation:

`:`, `,`, `=`, `<`, `>`, `(`, `)`, `{`, `}`, `[`, `]`, `->`, `?`, `+`, `*`

The following are valid whitespace punctuation:

`\n`, ` `

The `\n` literal emits a newline an indents to the start of the operation. An
example is shown below:

```tablegen
let assemblyFormat = [{
  `{` `\n` ` ` ` ` `this_is_on_a_newline` `\n` `}` attr-dict
}];
```

```mlir
%results = my.operation {
  this_is_on_a_newline
}
```

An empty literal \`\` may be used to remove a space that is inserted implicitly
after certain literal elements, such as `)`/`]`/etc. For example, "`]`" may
result in an output of `]` it is not the last element in the format. "`]` \`\`"
would trim the trailing space in this situation.

#### Variables

A variable is an entity that has been registered on the operation itself, i.e.
an argument(attribute or operand), region, result, successor, etc. In the
`CallOp` example above, the variables would be `$callee` and `$args`.

Attribute variables are printed with their respective value type, unless that
value type is buildable. In those cases, the type of the attribute is elided.

#### Custom Directives

The declarative assembly format specification allows for handling a large
majority of the common cases when formatting an operation. For the operations
that require or desire specifying parts of the operation in a form not supported
by the declarative syntax, custom directives may be specified. A custom
directive essentially allows for users to use C++ for printing and parsing
subsections of an otherwise declaratively specified format. Looking at the
specification of a custom directive above:

```
custom-directive ::= `custom` `<` UserDirective `>` `(` Params `)`
```

A custom directive has two main parts: The `UserDirective` and the `Params`. A
custom directive is transformed into a call to a `print*` and a `parse*` method
when generating the C++ code for the format. The `UserDirective` is an
identifier used as a suffix to these two calls, i.e., `custom<MyDirective>(...)`
would result in calls to `parseMyDirective` and `printMyDirective` within the
parser and printer respectively. `Params` may be any combination of variables
(i.e. Attribute, Operand, Successor, etc.), type directives, `attr-dict`, and
strings of C++ code. The type directives must refer to a variable, but that
variable need not also be a parameter to the custom directive.

The arguments to the `parse<UserDirective>` method are firstly a reference to
the `OpAsmParser`(`OpAsmParser &`), and secondly a set of output parameters
corresponding to the parameters specified in the format. The mapping of
declarative parameter to `parse` method argument is detailed below:

*   Attribute Variables
    -   Single: `<Attribute-Storage-Type>(e.g. Attribute) &`
    -   Optional: `<Attribute-Storage-Type>(e.g. Attribute) &`
*   Operand Variables
    -   Single: `OpAsmParser::UnresolvedOperand &`
    -   Optional: `Optional<OpAsmParser::UnresolvedOperand> &`
    -   Variadic: `SmallVectorImpl<OpAsmParser::UnresolvedOperand> &`
    -   VariadicOfVariadic:
        `SmallVectorImpl<SmallVector<OpAsmParser::UnresolvedOperand>> &`
*   Ref Directives
    -   A reference directive is passed to the parser using the same mapping as
        the input operand. For example, a single region would be passed as a
        `Region &`.
*   Region Variables
    -   Single: `Region &`
    -   Variadic: `SmallVectorImpl<std::unique_ptr<Region>> &`
*   Successor Variables
    -   Single: `Block *&`
    -   Variadic: `SmallVectorImpl<Block *> &`
*   Type Directives
    -   Single: `Type &`
    -   Optional: `Type &`
    -   Variadic: `SmallVectorImpl<Type> &`
    -   VariadicOfVariadic: `SmallVectorImpl<SmallVector<Type>> &`
*   `attr-dict` Directive: `NamedAttrList &`
*   `prop-dict` Directive: `OperationState &`

When a variable is optional, the value should only be specified if the variable
is present. Otherwise, the value should remain `None` or null.

The arguments to the `print<UserDirective>` method are firstly a reference to the
`OpAsmPrinter`(`OpAsmPrinter &`), second the op (e.g. `FooOp op` which can be
`Operation *op` alternatively), and finally a set of output parameters
corresponding to the parameters specified in the format. The mapping of
declarative parameter to `print` method argument is detailed below:

*   Attribute Variables
    -   Single: `<Attribute-Storage-Type>(e.g. Attribute)`
    -   Optional: `<Attribute-Storage-Type>(e.g. Attribute)`
*   Operand Variables
    -   Single: `Value`
    -   Optional: `Value`
    -   Variadic: `OperandRange`
    -   VariadicOfVariadic: `OperandRangeRange`
*   Ref Directives
    -   A reference directive is passed to the printer using the same mapping as
        the input operand. For example, a single region would be passed as a
        `Region &`.
*   Region Variables
    -   Single: `Region &`
    -   Variadic: `MutableArrayRef<Region>`
*   Successor Variables
    -   Single: `Block *`
    -   Variadic: `SuccessorRange`
*   Type Directives
    -   Single: `Type`
    -   Optional: `Type`
    -   Variadic: `TypeRange`
    -   VariadicOfVariadic: `TypeRangeRange`
*   `attr-dict` Directive: `DictionaryAttr`
*   `prop-dict` Directive: `FooOp::Properties`

When a variable is optional, the provided value may be null. When a variable is
referenced in a custom directive parameter using `ref`, it is passed in by
value. Referenced variables to `print<UserDirective>` are passed as the same as
bound variables, but referenced variables to `parse<UserDirective>` are passed
like to the printer.

A custom directive can take a string of C++ code as a parameter. The code is
pasted verbatim in the calls to the custom parser and printers, with the
substitutions `$_builder` and `$_ctxt`. String literals can be used to
parameterize custom directives.

#### Optional Groups

In certain situations operations may have "optional" information, e.g.
attributes or an empty set of variadic operands. In these situations a section
of the assembly format can be marked as `optional` based on the presence of this
information. An optional group is defined as follows:

```
optional-group: `(` then-elements `)` (`:` `(` else-elements `)`)? `?`
```

The elements of an optional group have the following requirements:

*   The first element of `then-elements` must either be a attribute, literal,
    operand, property, or region.
    -   This is because the first element must be optionally parsable.
    -   If a property is used, it must have an `optionalParser` defined and have a
        default value.
*   Exactly one argument variable or type directive within either
    `then-elements` or `else-elements` must be marked as the anchor of the
    group.
    -   The anchor is the element whose presence controls which elements
        should be printed/parsed.
    -   An element is marked as the anchor by adding a trailing `^`.
    -   The first element is *not* required to be the anchor of the group.
    -   When a non-variadic region anchors a group, the detector for printing
        the group is if the region is empty.
*   Literals, variables, custom directives, and type directives are the only
    valid elements within the group.
    -   Any attribute variable may be used, but only optional or default-valued
        attributes can be marked as the anchor. A default-valued anchor is
        considered present if it holds a value other than the default.
    -   Only variadic or optional results and operand arguments and can be used.
    -   All region variables can be used. When a non-variable length region is
        used, if the group is not present the region is empty.

An example of an operation with an optional group is `func.return`, which has a
variadic number of operands.

```tablegen
def ReturnOp : ... {
  let arguments = (ins Variadic<AnyType>:$operands);

  // We only print the operands and types if there are a non-zero number
  // of operands.
  let assemblyFormat = "attr-dict ($operands^ `:` type($operands))?";
}
```

##### Unit Attributes

In MLIR, the [`unit` Attribute](../Dialects/Builtin.md/#unitattr) is special in that it
only has one possible value, i.e. it derives meaning from its existence. When a
unit attribute is used to anchor an optional group and is not the first element
of the group, the presence of the unit attribute can be directly correlated with
the presence of the optional group itself. As such, in these situations the unit
attribute will not be printed or present in the output and will be automatically
inferred when parsing by the presence of the optional group itself.

For example, the following operation:

```tablegen
def FooOp : ... {
  let arguments = (ins UnitAttr:$is_read_only);

  let assemblyFormat = "attr-dict (`is_read_only` $is_read_only^)?";
}
```

would be formatted as such:

```mlir
// When the unit attribute is present:
foo.op is_read_only

// When the unit attribute is not present:
foo.op
```

The same logic applies to a `UnitProp`.

##### Optional "else" Group

Optional groups also have support for an "else" group of elements. These are
elements that are parsed/printed if the `anchor` element of the optional group
is *not* present. Unlike the main element group, the "else" group has no
restriction on the first element and none of the elements may act as the
`anchor` for the optional. An example is shown below:

```tablegen
def FooOp : ... {
  let arguments = (ins UnitAttr:$foo);

  let assemblyFormat = "attr-dict (`foo_is_present` $foo^):(`foo_is_absent`)?";
}
```

would be formatted as such:

```mlir
// When the `foo` attribute is present:
foo.op foo_is_present

// When the `foo` attribute is not present:
foo.op foo_is_absent
```

#### Requirements

The format specification has a certain set of requirements that must be adhered
to:

1.  The output and operation name are never shown as they are fixed and cannot
    be altered.
1.  All operands within the operation must appear within the format, either
    individually or with the `operands` directive.
1.  All regions within the operation must appear within the format, either
    individually or with the `regions` directive.
1.  All successors within the operation must appear within the format, either
    individually or with the `successors` directive.
1.  All operand and result types must appear within the format using the various
    `type` directives, either individually or with the `operands` or `results`
    directives.
1.  Unless all non-attribute properties appear in the format, the `prop-dict`
    directive must be present.
1.  The `attr-dict` directive must always be present.
1.  Must not contain overlapping information; e.g. multiple instances of
    'attr-dict', types, operands, etc.
    -   Note that `attr-dict` does not overlap with individual attributes. These
        attributes will simply be elided when printing the attribute dictionary.

##### Type Inference

One requirement of the format is that the types of operands and results must
always be present. In certain instances, the type of a variable may be deduced
via type constraints or other information available. In these cases, the type of
that variable may be elided from the format.

*   Buildable Types

Some type constraints may only have one representation, allowing for them to be
directly buildable; for example the `I32` or `Index` types. Types in `ODS` may
mark themselves as buildable by setting the `builderCall` field or inheriting
from the `BuildableType` class.

*   Trait Equality Constraints

There are many operations that have known type equality constraints registered
as traits on the operation; for example the true, false, and result values of a
`select` operation often have the same type. The assembly format may inspect
these equal constraints to discern the types of missing variables. The currently
supported traits are: `AllTypesMatch`, `TypesMatchWith`, `SameTypeOperands`, and
`SameOperandsAndResultType`.

*   InferTypeOpInterface

Operations that implement `InferTypeOpInterface` can omit their result types in
their assembly format since the result types can be inferred from the operands.

### `hasCanonicalizer`

This boolean field indicate whether canonicalization patterns have been defined
for this operation. If it is `1`, then `::getCanonicalizationPatterns()` should
be defined.

### `hasCanonicalizeMethod`

When this boolean field is set to `true`, it indicates that the op implements a
`canonicalize` method for simple "matchAndRewrite" style canonicalization
patterns. If `hasCanonicalizer` is 0, then an implementation of
`::getCanonicalizationPatterns()` is implemented to call this function.

### `hasFolder`

This boolean field indicate whether general folding rules have been defined for
this operation. If it is `1`, then `::fold()` should be defined.

### Extra declarations

One of the goals of table-driven op definition is to auto-generate as much logic
and methods needed for each op as possible. With that said, there will always be
long-tail cases that won't be covered. For such cases, you can use
`extraClassDeclaration`. Code in `extraClassDeclaration` will be copied
literally to the generated C++ op class.

Note that `extraClassDeclaration` is a mechanism intended for long-tail cases by
power users; for not-yet-implemented widely-applicable cases, improving the
infrastructure is preferable.

### Extra definitions

When defining base op classes in TableGen that are inherited many times by
different ops, users may want to provide common definitions of utility and
interface functions. However, many of these definitions may not be desirable or
possible in `extraClassDeclaration`, which append them to the op's C++ class
declaration. In these cases, users can add an `extraClassDefinition` to define
code that is added to the generated source file inside the op's C++ namespace.
The substitution `$cppClass` is replaced by the op's C++ class name.

### Generated C++ code

[OpDefinitionsGen][OpDefinitionsGen] processes the op definition spec file and
generates two files containing the corresponding C++ code: one for declarations,
the other for definitions. The former is generated via the `-gen-op-decls`
command-line option, while the latter is via the `-gen-op-defs` option.

The definition file contains all the op method definitions, which can be
included and enabled by defining `GET_OP_CLASSES`. For each operation,
OpDefinitionsGen generates an operation class and an
[operand adaptor](#operand-adaptors) class. Besides, it also contains a
comma-separated list of all defined ops, which can be included and enabled by
defining `GET_OP_LIST`.

#### Class name and namespaces

For each operation, its generated C++ class name is the symbol `def`ed with
TableGen with dialect prefix removed. The first `_` serves as the delimiter. For
example, for `def TF_AddOp`, the C++ class name would be `AddOp`. We remove the
`TF` prefix because it is for scoping ops; other dialects may as well define
their own `AddOp`s.

The namespaces of the generated C++ class will come from the dialect's
`cppNamespace` field. For example, if a dialect's `cppNamespace` is `A::B`, then
an op of that dialect will be placed in `namespace A { namespace B { ... } }`.
If a dialect does not specify a `cppNamespace`, we then use the dialect's name
as the namespace.

This means the qualified name of the generated C++ class does not necessarily
match exactly with the operation name as explained in
[Operation name](#operation-name). This is to allow flexible naming to satisfy
coding style requirements.

#### Operand adaptors

For each operation, we automatically generate an _operand adaptor_. This class
solves the problem of accessing operands provided as a list of `Value`s without
using "magic" constants. The operand adaptor takes a reference to an array of
`Value` and provides methods with the same names as those in the operation class
to access them. For example, for a binary arithmetic operation, it may provide
`.lhs()` to access the first operand and `.rhs()` to access the second operand.

The operand adaptor class lives in the same namespace as the operation class,
and has the name of the operation followed by `Adaptor` as well as an alias
`Adaptor` inside the op class.

Operand adaptors can be used in function templates that also process operations:

```c++
template <typename BinaryOpTy>
std::pair<Value, Value> zip(BinaryOpTy &&op) {
  return std::make_pair(op.lhs(), op.rhs());;
}

void process(AddOp op, ArrayRef<Value> newOperands) {
  zip(op);
  zip(Adaptor<AddOp>(newOperands));
  /*...*/
}
```

#### Sharded Operation Definitions

Large dialects with many operations may struggle with C++ compile time of
generated op definitions, due to large compilation units. `mlir-tblgen`
provides the ability to shard op definitions by splitting them up evenly
by passing `-op-shard-count` to `-gen-op-defs` and `-gen-op-decls`. The tool
will generate a single include file for the definitions broken up by
`GET_OP_DEFS_${N}` where `${N}` is the shard number. A shard can be compiled in
a single compilation unit by adding a file like this to your dialect library:

```c++
#include "mlir/IR/Operation.h"
// Add any other required includes.

// Utilities shared by generated op definitions: custom directive parsers,
// printers, etc.
#include "OpUtils.h"

#define GET_OP_DEFS_0
#include "MyDialectOps.cpp.inc"
```

Note: this requires restructing shared utility functions within the dialect
library so they can be shared by multiple compilation units. I.e. instead of
defining `static` methods in the same source file, you should declare them in a
shared header and define them in their own source file.

The op registration hooks are also sharded, because the template instantiation
can take a very long time to compile. Operations should be registered in your
dialect like:

```c++
void MyDialect::initialize() {
  registerMyDialectOperations(this);
}
```

CMake and Bazel functions are included to make sharding dialects easier.
Assuming you have organized your operation utility functions into their own
header, define a file that looks like the one above, but without the `#define`:

```c++
// MyDialectOps.cpp
#include "mlir/IR/Operation.h"

#include "OpUtils.h"

#include "MyDialectOps.cpp.inc"
```

In CMake, remove the manual `mlir_tablegen` invocations and replace them with:

```cmake
set(LLVM_TARGET_DEFINITIONS MyDialectOps.td)
add_sharded_ops(MyDialectOps 8) # shard the op definitions by 8

add_mlir_library(MyDialect
  MyDialect.cpp
  MyDialectOpDefs.cpp
  ${SHARDED_SRCS}

  DEPENDS
  MLIRTestOpsShardGen
)
```

This will automatically duplicate the `MyDialectOps.cpp` source file and add the
`#define` up the number of shards indicated.

It is recommended that any out-of-line op member functions (like verifiers) be
defined in a separate source file. In this example, it is called
`MyDialectOpDefs.cpp`.

In Bazel, remove the `-gen-op-defs` and `-gen-op-decls` invocations, and add

```bazel
gentbl_sharded_ops(
    name = "MyDialectOpSrcs",
    hdr_out = "MyDialectOps.h.inc",
    shard_count = 8,
    sharder = "//mlir:mlir-src-sharder",
    src_file = "MyDialectOps.cpp",
    src_out = "MyDialectOps.cpp.inc",
    tblgen = "//mlir:mlir-tblgen",
    td_file = "MyDialectOps.td",
    deps = [":MyDialectOpsTdFiles"],
)

cc_library(
    name = "MyDialect",
    srcs = glob(["MyDialect/*.cpp"]) + [":MyDialectOpSrcs"]
)
```

## Constraints

Constraint is a core concept in table-driven operation definition: operation
verification and graph operation matching are all based on satisfying
constraints. So both the operation definition and rewrite rules specification
significantly involve writing constraints. We have the `Constraint` class in
[`OpBase.td`][OpBase] as the common base class for all constraints.

An operation's constraint can cover different range; it may

*   Only concern a single attribute (e.g. being a 32-bit integer greater than
    5),
*   Multiple operands and results (e.g., the 1st result's shape must be the same
    as the 1st operand), or
*   Intrinsic to the operation itself (e.g., having no side effect).

We call them as single-entity constraint, multi-entity constraint, and traits,
respectively.

### Single-entity constraint

Constraints scoped to a single operand, attribute, or result are specified at
the entity's declaration place as described in
[Operation arguments](#operation-arguments) and
[Operation results](#operation-results).

To help modelling constraints of common types, a set of `TypeConstraint`s are
created; they are the `Type` subclass hierarchy. It includes `F32` for the
constraints of being a float, `TensorOf<[F32]>` for the constraints of being a
float tensor, and so on.

Similarly, a set of `AttrConstraint`s are created for helping modelling
constraints of common attribute kinds. They are the `Attr` subclass hierarchy.
It includes `F32Attr` for the constraints of being a float attribute,
`F32ArrayAttr` for the constraints of being a float array attribute, and so on.

### Multi-entity constraint

Constraints involving more than one operand/attribute/result are quite common on
operations, like the element type and shape relation between operands and
results. These constraints should be specified as the `Op` class template
parameter as described in
[Operation traits and constraints](#operation-traits-and-constraints).

Multi-entity constraints are modeled as `PredOpTrait` (a subclass of `Trait`)
in [`OpBase.td`][OpBase].A bunch of constraint primitives are provided to help
specification. See [`OpBase.td`][OpBase] for the complete list.

### Trait

Traits are intrinsic properties of the operation like having side effect or not,
commutative or not, whether is a terminator, etc. These constraints should be
specified as the `Op` class template parameter as described in
[Operation traits and constraints](#operation-traits-and-constraints).

Traits are modeled as `NativeTrait` (a subclass of `Trait`) in
[`OpBase.td`][OpBase]. They are backed and will be translated into the
corresponding C++ `mlir::OpTrait` classes.

### How to specify new constraint

To write a constraint, you need to provide its predicates and give it a
descriptive name. Predicates, modeled with the `Pred` class, are the workhorse
for composing constraints. The predicate for a constraint is typically built up
in a nested manner, using the two categories of predicates:

1.  `CPred`: the primitive leaf predicate.
2.  Compound predicate: a predicate composed from child predicates using
    predicate combiners (conjunction: `And`, disjunction: `Or`, negation: `Neg`,
    substitution: `SubstLeaves`, concatenation: `Concat`).

`CPred` is the basis for composing more complex predicates. It is the "atom"
predicate from the perspective of TableGen and the "interface" between TableGen
and C++. What is inside is already C++ code, which will be treated as opaque
strings with special placeholders to be substituted.

You can put any C++ code that returns a boolean value inside a `CPred`,
including evaluating expressions, calling functions, calling class methods, and
so on.

To help interaction with the C++ environment, there are a few special
placeholders provided to refer to entities in the context where this predicate
is used. They serve as "hooks" to the enclosing environment. This includes
`$_builder`, `$_op`, and `$_self`:

*   `$_builder` will be replaced by a `mlir::Builder` instance so that you can
    access common build methods.
*   `$_op` will be replaced by the current operation so that you can access
    information of the current operation.
*   `$_self` will be replaced with the entity this predicate is attached to.
    E.g., `BoolAttr` is an attribute constraint that wraps a
    `CPred<"isa<BoolAttr>($_self)">`. Then for `BoolAttr:$attr`,`$_self` will be
    replaced by `$attr`. For type constraints, it's a little bit special since
    we want the constraints on each type definition reads naturally and we want
    to attach type constraints directly to an operand/result, `$_self` will be
    replaced by the operand/result's type. E.g., for `F32` in `F32:$operand`,
    its `$_self` will be expanded as `operand(...).getType()`.

TODO: Reconsider the leading symbol for special placeholders. Eventually we want
to allow referencing operand/result `$-name`s; such `$-name`s can start with
underscore.

For example, to write an attribute `attr` is an `IntegerAttr`, in C++ you can
just call `isa<IntegerAttr>(attr)`. The code can be wrapped in a `CPred` as
`isa<IntegerAttr>($_self)`, with `$_self` as the special placeholder to be
replaced by the current attribute `attr` at expansion time.

For more complicated predicates, you can wrap it in a single `CPred`, or you can
use predicate combiners to combine them. For example, to write the constraint
that an attribute `attr` is a 32-bit or 64-bit integer, you can write it as

```tablegen
And<[
  CPred<"$isa<IntegerAttr>(_self)()">,
  Or<[
    CPred<"cast<IntegerAttr>($_self).getType().isInteger(32)">,
    CPred<"cast<IntegerAttr>($_self).getType().isInteger(64)">
  ]>
]>
```

(Note that the above is just to show with a familiar example how you can use
`CPred` and predicate combiners to write complicated predicates. For integer
attributes specifically, [`OpBase.td`][OpBase] already defines `I32Attr` and
`I64Attr`. So you can actually reuse them to write it as `Or<[I32Attr.predicate,
I64Attr.predicate]>`.)

TODO: Build up a library of reusable primitive constraints

If the predicate is very complex to write with `CPred` together with predicate
combiners, you can also write it as a normal C++ function and use the `CPred` as
a way to "invoke" the function. For example, to verify an attribute `attr` has
some property, you can write a C++ function like

```cpp
bool HasSomeProperty(Attribute attr) { ... }
```

and then define the op as:

```tablegen
def HasSomeProperty : AttrConstraint<CPred<"HasSomeProperty($_self)">,
                                     "has some property">;

def MyOp : Op<...> {
  let arguments = (ins
    ...
    HasSomeProperty:$attr
  );
}
```

As to whether we should define the predicate using a single `CPred` wrapping the
whole expression, multiple `CPred`s with predicate combiners, or a single
`CPred` "invoking" a function, there are no clear-cut criteria. Defining using
`CPred` and predicate combiners is preferable since it exposes more information
(instead hiding all the logic behind a C++ function) into the op definition spec
so that it can potentially drive more auto-generation cases. But it will require
a nice library of common predicates as the building blocks to avoid the
duplication, which is being worked on right now.

## Attribute Definition

An attribute is a compile-time known constant of an operation.

ODS provides attribute wrappers over C++ attribute classes. There are a few
common C++ [attribute classes][AttrClasses] defined in MLIR's core IR library
and one is free to define dialect-specific attribute classes. ODS allows one to
use these attributes in TableGen to define operations, potentially with more
fine-grained constraints. For example, `StrAttr` directly maps to `StringAttr`;
`F32Attr`/`F64Attr` requires the `FloatAttr` to additionally be of a certain
bitwidth.

ODS attributes are defined as having a storage type (corresponding to a backing
`mlir::Attribute` that _stores_ the attribute), a return type (corresponding to
the C++ _return_ type of the generated helper getters) as well as a method
to convert between the internal storage and the helper method.

### Attribute decorators

There are a few important attribute adapters/decorators/modifiers that can be
applied to ODS attributes to specify common additional properties like
optionality, default values, etc.:

*   `DefaultValuedAttr`: specifies the
    [default value](#attributes-with-default-values) for an attribute.
*   `OptionalAttr`: specifies an attribute as [optional](#optional-attributes).
*   `ConfinedAttr`: adapts an attribute with
    [further constraints](#confining-attributes).
*   `AllAttrOf`: adapts an attribute with
    [multiple constraints](#combining-constraints).

## Enum definition

MLIR is capabable of generating C++ enums, both those that represent a set
of values drawn from a list or that can hold a combination of flags
using the `IntEnum` and `BitEnum` classes, respectively.

All these `IntEnum` and `BitEnum` classes require fully specifying all of the allowed
cases via a `EnumCase` or `BitEnumCase` subclass, respectively. With this, ODS is able to
generate additional verification to only accept allowed cases. To facilitate the
interaction between tablegen enums and the attributes or properties that wrap them and
to make them easier to use in C++, the
[`EnumsGen`][EnumsGen] TableGen backend can generate a few common utilities: a
C++ enum class, `llvm::DenseMapInfo` for the enum class, conversion functions
from/to strings. This is controlled via the `-gen-enum-decls` and
`-gen-enum-defs` command-line options of `mlir-tblgen`.

For example, given the following `EnumAttr`:

```tablegen
def Case15: I32EnumCase<"Case15", 15>;
def Case20: I32EnumCase<"Case20", 20>;

def MyIntEnum: I32Enum<"MyIntEnum", "An example int enum",
                           [Case15, Case20]> {
  let cppNamespace = "Outer::Inner";
  let stringToSymbolFnName = "ConvertToEnum";
  let symbolToStringFnName = "ConvertToString";
}
```

The following will be generated via `mlir-tblgen -gen-enum-decls`:

```c++
namespace Outer {
namespace Inner {
// An example int enum
enum class MyIntEnum : uint32_t {
  Case15 = 15,
  Case20 = 20,
};

std::optional<MyIntEnum> symbolizeMyIntEnum(uint32_t);
llvm::StringRef ConvertToString(MyIntEnum);
std::optional<MyIntEnum> ConvertToEnum(llvm::StringRef);
inline constexpr unsigned getMaxEnumValForMyIntEnum() {
  return 20;
}

} // namespace Inner
} // namespace Outer

namespace llvm {
template<> struct DenseMapInfo<Outer::Inner::MyIntEnum> {
  using StorageInfo = llvm::DenseMapInfo<uint32_t>;

  static inline Outer::Inner::MyIntEnum getEmptyKey() {
    return static_cast<Outer::Inner::MyIntEnum>(StorageInfo::getEmptyKey());
  }

  static inline Outer::Inner::MyIntEnum getTombstoneKey() {
    return static_cast<Outer::Inner::MyIntEnum>(StorageInfo::getTombstoneKey());
  }

  static unsigned getHashValue(const Outer::Inner::MyIntEnum &val) {
    return StorageInfo::getHashValue(static_cast<uint32_t>(val));
  }

  static bool isEqual(const Outer::Inner::MyIntEnum &lhs, const Outer::Inner::MyIntEnum &rhs) {
    return lhs == rhs;
  }
};
}
```

The following will be generated via `mlir-tblgen -gen-enum-defs`:

```c++
namespace Outer {
namespace Inner {
llvm::StringRef ConvertToString(MyIntEnum val) {
  switch (val) {
    case MyIntEnum::Case15: return "Case15";
    case MyIntEnum::Case20: return "Case20";
  }
  return "";
}

std::optional<MyIntEnum> ConvertToEnum(llvm::StringRef str) {
  return llvm::StringSwitch<std::optional<MyIntEnum>>(str)
      .Case("Case15", MyIntEnum::Case15)
      .Case("Case20", MyIntEnum::Case20)
      .Default(std::nullopt);
}
std::optional<MyIntEnum> symbolizeMyIntEnum(uint32_t value) {
  switch (value) {
  case 15: return MyIntEnum::Case15;
  case 20: return MyIntEnum::Case20;
  default: return std::nullopt;
  }
}

} // namespace Inner
} // namespace Outer
```

Similarly for the following `BitEnumAttr` definition:

```tablegen
def None: I32BitEnumCaseNone<"None">;
def Bit0: I32BitEnumCaseBit<"Bit0", 0, "tagged">;
def Bit1: I32BitEnumCaseBit<"Bit1", 1>;
def Bit2: I32BitEnumCaseBit<"Bit2", 2>;
def Bit3: I32BitEnumCaseBit<"Bit3", 3>;

def MyBitEnum: I32BitEnum<"MyBitEnum", "An example bit enum",
                           [None, Bit0, Bit1, Bit2, Bit3]> {
  // Note: this is the default value, and is listed for illustrative purposes.
  let separator = "|";
}
```

We can have:

```c++
// An example bit enum
enum class MyBitEnum : uint32_t {
  None = 0,
  Bit0 = 1,
  Bit1 = 2,
  Bit2 = 4,
  Bit3 = 8,
};

std::optional<MyBitEnum> symbolizeMyBitEnum(uint32_t);
std::string stringifyMyBitEnum(MyBitEnum);
std::optional<MyBitEnum> symbolizeMyBitEnum(llvm::StringRef);

inline constexpr MyBitEnum operator|(MyBitEnum a, MyBitEnum b) {
  return static_cast<MyBitEnum>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline constexpr MyBitEnum operator&(MyBitEnum a, MyBitEnum b) {
  return static_cast<MyBitEnum>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline constexpr MyBitEnum operator^(MyBitEnum a, MyBitEnum b) {
  return static_cast<MyBitEnum>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
}
inline constexpr MyBitEnum operator~(MyBitEnum bits) {
  // Ensure only bits that can be present in the enum are set
  return static_cast<MyBitEnum>(~static_cast<uint32_t>(bits) & static_cast<uint32_t>(15u));
}
inline constexpr bool bitEnumContainsAll(MyBitEnum bits, MyBitEnum bit) {
  return (bits & bit) == bit;
}
inline constexpr bool bitEnumContainsAny(MyBitEnum bits, MyBitEnum bit) {
  return (static_cast<uint32_t>(bits) & static_cast<uint32_t>(bit)) != 0;
}
inline constexpr MyBitEnum bitEnumClear(MyBitEnum bits, MyBitEnum bit) {
  return bits & ~bit;
}

inline std::string stringifyEnum(MyBitEnum enumValue) {
  return stringifyMyBitEnum(enumValue);
}

template <typename EnumType>
::std::optional<EnumType> symbolizeEnum(::llvm::StringRef);

template <>
inline ::std::optional<MyBitEnum> symbolizeEnum<MyBitEnum>(::llvm::StringRef str) {
  return symbolizeMyBitEnum(str);
}

namespace llvm {
template<> struct DenseMapInfo<::MyBitEnum> {
  using StorageInfo = llvm::DenseMapInfo<uint32_t>;

  static inline ::MyBitEnum getEmptyKey() {
    return static_cast<::MyBitEnum>(StorageInfo::getEmptyKey());
  }

  static inline ::MyBitEnum getTombstoneKey() {
    return static_cast<::MyBitEnum>(StorageInfo::getTombstoneKey());
  }

  static unsigned getHashValue(const ::MyBitEnum &val) {
    return StorageInfo::getHashValue(static_cast<uint32_t>(val));
  }

  static bool isEqual(const ::MyBitEnum &lhs, const ::MyBitEnum &rhs) {
    return lhs == rhs;
  }
};
```

```c++
std::string stringifyMyBitEnum(MyBitEnum symbol) {
  auto val = static_cast<uint32_t>(symbol);
  assert(15u == (15u | val) && "invalid bits set in bit enum");
  // Special case for all bits unset.
  if (val == 0) return "None";
  llvm::SmallVector<llvm::StringRef, 2> strs;
  if (1u == (1u & val)) { strs.push_back("tagged"); }
  if (2u == (2u & val)) { strs.push_back("Bit1"); }
  if (4u == (4u & val)) { strs.push_back("Bit2"); }
  if (8u == (8u & val)) { strs.push_back("Bit3"); }

  return llvm::join(strs, "|");
}

std::optional<MyBitEnum> symbolizeMyBitEnum(llvm::StringRef str) {
  // Special case for all bits unset.
  if (str == "None") return MyBitEnum::None;

  llvm::SmallVector<llvm::StringRef, 2> symbols;
  str.split(symbols, "|");

  uint32_t val = 0;
  for (auto symbol : symbols) {
    auto bit = llvm::StringSwitch<std::optional<uint32_t>>(symbol)
      .Case("tagged", 1)
      .Case("Bit1", 2)
      .Case("Bit2", 4)
      .Case("Bit3", 8)
      .Default(std::nullopt);
    if (bit) { val |= *bit; } else { return std::nullopt; }
  }
  return static_cast<MyBitEnum>(val);
}

std::optional<MyBitEnum> symbolizeMyBitEnum(uint32_t value) {
  // Special case for all bits unset.
  if (value == 0) return MyBitEnum::None;

  if (value & ~static_cast<uint32_t>(15u)) return std::nullopt;
  return static_cast<MyBitEnum>(value);
}
```

### Wrapping enums in attributes

There are several mechanisms for creating an `Attribute` whose values are
taken from a `*Enum`.

The most common of these is to use the `EnumAttr` class, which takes
an `EnumInfo` (either a `IntEnum` or `BitEnum`) as a parameter and constructs
an attribute that holds one argument - value of the enum. This attribute
is defined within a dialect and can have its assembly format customized to,
for example, print angle brackets around the enum value or assign a mnemonic.

An older form involves using the `*IntEnumAttr` and `*BitEnumATtr` classes
and their corresponding `*EnumAttrCase` classes (which can be used
anywhere a `*EnumCase` is needed). These classes store their values
as a `SignlessIntegerAttr` of their bitwidth, imposing the constraint on it
that it has a value within the valid range of the enum. If their
`genSpecializedAttr` parameter is set, they will also generate a
wrapper attribute instead of using a bare signless integer attribute
for storage.

### Enum properties

Enums can be wrapped in properties so that they can be stored inline.
This causes a value of the enum's C++ class to become a member of the operation's
property struct and for the operation's verifier to check that the enum's value
is a valid value for the enum.

The basic wrapper is `EnumProp`, which simply takes an `EnumInfo`.

A less ambiguous syntax, namely putting a mnemonic and `<>`s surrounding
the enum is generated with `NamedEnumProp`, which takes a `*EnumInfo`
and a mnemonic string, which becomes part of the property's syntax.

Both of these `EnumProp` types have a `*EnumPropWithAttrForm`, which allows for
transparently upgrading from `EnumAttr`s and optionally retaining those
attributes in the generic form.

## Debugging Tips

### Run `mlir-tblgen` to see the generated content

TableGen syntax sometimes can be obscure; reading the generated content can be a
very helpful way to understand and debug issues. To build `mlir-tblgen`, run
`cmake --build . --target mlir-tblgen` in your build directory and find the
`mlir-tblgen` binary in the `bin/` subdirectory. All the supported generators
can be found via `mlir-tblgen --help`. For example, `--gen-op-decls` and
`--gen-op-defs` as explained in [Generated C++ code](#generated-c-code).

To see the generated code, invoke `mlir-tblgen` with a specific generator by
providing include paths via `-I`. For example,

```sh
# To see op C++ class declaration
mlir-tblgen --gen-op-decls -I /path/to/mlir/include /path/to/input/td/file
# To see op C++ class definition
mlir-tblgen --gen-op-defs -I /path/to/mlir/include /path/to/input/td/file
# To see op documentation
mlir-tblgen --gen-dialect-doc -I /path/to/mlir/include /path/to/input/td/file

# To see op interface C++ class declaration
mlir-tblgen --gen-op-interface-decls -I /path/to/mlir/include /path/to/input/td/file
# To see op interface C++ class definition
mlir-tblgen --gen-op-interface-defs -I /path/to/mlir/include /path/to/input/td/file
# To see op interface documentation
mlir-tblgen --gen-op-interface-doc -I /path/to/mlir/include /path/to/input/td/file
```

## Appendix

### Reporting deprecation in TableGen

Classes/defs can be marked as deprecated by using the `Deprecate` helper class,
e.g.,

```tablegen
def OpTraitA : NativeOpTrait<"OpTraitA">, Deprecated<"use `bar` instead">;
```

would result in marking `OpTraitA` as deprecated and mlir-tblgen can emit a
warning (default) or error (depending on `-on-deprecated` flag) to make
deprecated state known.

### Reporting deprecation in C++

TableGen generated C++ entities, such as classes, functions or methods, can be
marked as deprecated using the `CppDeprecated` mixin:

```tablegen
def MyOp : Op<MyDialect, "my.op">, CppDeprecated<"use 'your.op' instead">;
```

This differs to the deprecation mechanic for TableGen, in that no warning is
emitted by mlir-tblgen. Rather, a warning with the given reason is emitted by
the C++ compiler on use of the given entity.

To allow more convenient syntax, helper classes exist for TableGen classes
which are commonly used as anonymous definitions. These currently include:

* `DeprecatedOpBuilder`: Can be used in place of `OpBuilder` with the same
  arguments except taking the reason as first argument, e.g.
  `DeprecatedOpBuilder<"use 'build' with foo instead", (ins "int":$bar)>`

Note: Support for the `CppDeprecated` mechanism has to be implemented by
every code generator separately.

### Requirements and existing mechanisms analysis

The op description should be as declarative as possible to allow a wide range of
tools to work with them and query methods generated from them. In particular
this means specifying traits, constraints and shape inference information in a
way that is easily analyzable (e.g., avoid opaque calls to C++ functions where
possible).

We considered the approaches of several contemporary systems and focused on
requirements that were desirable:

*   Ops registered using a registry separate from C++ code.
    *   Unknown ops are allowed in MLIR, so ops need not be registered. The
        ability of the compiler to optimize those ops or graphs containing those
        ops is constrained but correct.
    *   The current proposal does not include a runtime op description, but it
        does not preclude such description, it can be added later.
    *   The op registry is essential for generating C++ classes that make
        manipulating ops, verifying correct construction etc. in C++ easier by
        providing a typed representation and accessors.
*   The op registry will be defined in
    [TableGen](https://llvm.org/docs/TableGen/index.html) and be used to
    generate C++ classes and utility functions
    (builder/verifier/parser/printer).
    *   TableGen is a modelling specification language used by LLVM's backends
        and fits in well with trait-based modelling. This is an implementation
        decision and there are alternative ways of doing this. But the
        specification language is good for the requirements of modelling the
        traits (as seen from usage in LLVM processor backend modelling) and easy
        to extend, so a practical choice. If another good option comes up, we
        will consider it.
*   MLIR allows both defined and undefined ops.
    *   Defined ops should have fixed semantics and could have a corresponding
        reference implementation defined.
    *   Dialects are under full control of the dialect owner and normally live
        with the framework of the dialect.
*   The op's traits (e.g., commutative) are modelled along with the op in the
    registry.
*   The op's operand/return type constraints are modelled along with the op in
    the registry (see [Shape inference](../ShapeInference.md) discussion below),
    this allows (e.g.) optimized concise syntax in textual dumps.
*   Behavior of the op is documented along with the op with a summary and a
    description. The description is written in markdown and extracted for
    inclusion in the generated LangRef section of the dialect.
*   The generic assembly form of printing and parsing is available as normal,
    but a custom parser and printer can either be specified or automatically
    generated from an optional string representation showing the mapping of the
    "assembly" string to operands/type.
    *   Parser-level remappings (e.g., `eq` to enum) will be supported as part
        of the parser generation.
*   Matching patterns are specified separately from the op description.
    *   Contrasted with LLVM there is no "base" set of ops that every backend
        needs to be aware of. Instead there are many different dialects and the
        transformations/legalizations between these dialects form a graph of
        transformations.
*   Reference implementation may be provided along with the op definition.

    *   The reference implementation may be in terms of either standard ops or
        other reference implementations.

    TODO: document expectation if the dependent op's definition changes.

[TableGen]: https://llvm.org/docs/TableGen/index.html
[TableGenProgRef]: https://llvm.org/docs/TableGen/ProgRef.html
[TableGenBackend]: https://llvm.org/docs/TableGen/BackEnds.html#introduction
[OpBase]: https://github.com/llvm/llvm-project/blob/main/mlir/include/mlir/IR/OpBase.td
[OpDefinitionsGen]: https://github.com/llvm/llvm-project/blob/main/mlir/tools/mlir-tblgen/OpDefinitionsGen.cpp
[EnumsGen]: https://github.com/llvm/llvm-project/blob/main/mlir/tools/mlir-tblgen/EnumsGen.cpp
[StringAttr]: ../Dialects/Builtin.md/#stringattr
[IntegerAttr]: ../Dialects/Builtin.md/#integertype
[AttrClasses]: https://github.com/llvm/llvm-project/blob/main/mlir/include/mlir/IR/Attributes.h
