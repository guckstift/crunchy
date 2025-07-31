# crunchy

*crunchy* is my work-in-progress programming language. It aims to be a natively compiled, statically typed but also garbage collected language that feels familiar to JavaScript/TypeScript or Python developers but compiles to plain C code.

## Building

This line creates the crunchy compiler `./build/crunchy`:

```
make
```

## Usage

This line creates a C file beside the input file with the name `<input-file-name>.c`.

```
./build/crunchy <input-file-name>
```

Example:

```
./build/crunchy ./test.cr
```

The generated C file `./test.cr.c` can then be compiled via e.g. `gcc`.

```
gcc -o test ./test.cr.c
```

## Current language status

Here I document every feature implemented so far. This list should grow with each new commit.

* comments
  * single line: `#` [^`\n`]*
* types
  * `int` : 64 bit signed integer
  * `bool` : boolean value, `true` or `false`
  * `string` : immutable array of byte characters
  * `function` : type of a function
  * array type
    * type<sub>item</sub> `[` `]`
    * mutable, dynamic sequence of homogenous values
* expressions
  * decimal integer literal : `[0-9]+`
  * boolean literal : `true` or `false`
  * string literal : `"` string-characters `"`
    * string-characters are all passing C's `isprint()` function except `"`
    * escape sequence allowed: `\"`
  * variables
    * must be used not before the end of their own declaration
  * binary operators
    * expression<sub>left</sub> `+` expression<sub>right</sub>
      * int/bool + int/bool = int
      * string + string = string
  * function call
    * expression `(` `)`
      * expression must be callable (function name or function pointer)
* conversion
  * `int` to `bool` : x = 0 => `false`, otherwise `true`
  * `bool` to `int` : x = `false` => 0, x = `true` => 1
  * `string` can not be converted from or to

### Statements

#### Variable declarations

* `var` IDENTIFIER `:` type `;`
  * the initial value is implicitly `0` for `int` and `false` for `bool`
* `var` IDENTIFIER `=` expression `;`
  * the variable type is inferred from the initializer expression
* `var` IDENTIFIER `:` type `=` expression `;`
  * the initializer expression is possibly converted to the specified type

#### Function declarations

* `function` IDENTIFIER `(` `)` `{` statement* `}`

#### Assignments

* expression<sub>target</sub> `=` expression<sub>value</sub> `;`
  * the target must be an L-value

#### Call statements

* call-expression `;`

#### print statements

* `print` expression `;`
  * prints the value of expression on a separate line

#### if statements

* `if` expression `{` statement* `}` ( `else` `{` statement* `}` )?