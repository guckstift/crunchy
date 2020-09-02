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

Variables must be declared before being used. A declaration looks like this:

```
var ident : type;
```

"ident" is a self-chosen name containing only lowercase or uppercase letters or decimal digits while it should not start with a
digit. "type" is one of the primitive type names listed above.

After a variable has been declared it can be assigned values with this syntax:

```
ident = expr;
```

"expr" can be a literal, it can be another variable name and in case the expression's type is "number" the expression can be
prefixed with `-` to negate it. The expression's type must match the type of the target variable.

A number literal is formed by a string of decimal digits. This number should not be bigger than the hexadecimal 0xffFFffFF. Although it
is stored as a 64-bit float it is only possible to notate numbers of unsigned 32-bit integer type.

A string literal is enclosed in double-quotes `"`. Between those quotes can only appear ASCII characters that are not control
characters, so e.g. no newlines or tabs are allowed in there.
