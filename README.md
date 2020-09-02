# crunchy

"crunchy" is a programming language. This repository contains the crunchy compiler. The compiler is currently in "proto" phase.
This is the first phase of development.

## proto

"proto" is the current phase of the compiler's development. The crunchy proto compiler is written in Python 3. It takes a single
source file as input and transpiles it into a C source code file. Then it passes this file to GCC to compile it into an
executable.

The language has 3 different primitive types and no more other types. The primitive types are:

* "int" - a 32-bit signed integer number
* "bool" - a boolean value, one of either "true" or "false", stored as a single byte containing 0x00 or 0x01
* "string" - an immutable fixed-size array of ASCII characters

Variables must be declared before being used. A declaration might look like this:

```
var ident : type;
```

Optionally a declaration can also contain an initializer like so:

```
var ident : type = expr;
```

In the last case the type specification can be left out and the compiler can infer the type from the initializer expression:

```
var ident = expr;
```

"ident" is a self-chosen name containing only lowercase or uppercase letters or decimal digits while it should not start with a
digit. "type" is one of the primitive type names listed above.

After a variable has been declared it can be assigned values with this syntax:

```
ident = expr;
```

"expr" can be a literal, it can be another variable name or it can be a more complex expression combining simpler expressions
with operators. The expression's type must match the type of the target variable.

An integer literal is formed by a string of decimal digits. This integer should not be bigger than `2^32 - 1`.

A string literal is enclosed in double-quotes `"`. Between those quotes only ASCII characters that are not control characters
can appear, so e.g. no newlines or tabs are allowed in a string literal.

Two or more expressions of type "int" can be combined with `+`, `-` or `*` together. The `*` which forms a multiplication has
higher precedence over `+` or `-` which form an addition or subtraction.

Two "int" values can be compared with `<` or `>`. The result is a "bool" value.

Other statement types include the "print" statement, the "if" or "if-else" statement or the "while" statement.

The "print" statement prints a value of any type to the standard output onto a separate line:

```
print expr;
```

The "if" statement looks like this:

```
if cond {
	# if-body
}
```

Optionally it can include an "else" branch:

```
if cond {
	# if-body
} else {
	# else-body
}
```

The "while" statement looks similar to an "if" statement:

```
while cond {
	# while-body
	# executed as often as cond is "true"
}
```

In all the above statements "cond" serves as the branch or loop condition. It must be of type "bool".

Inside a "while", "if" or "else" body more statements can appear. The body establishes a new scope. Variables declared inside
this scope are not accessible from outside the body. But variables of the outer scopes are accessible from in there.

