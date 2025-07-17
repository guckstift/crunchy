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

* variable declarations
  * `var` IDENTIFIER `:` type `;`
    * the initial value is implicitly `0` for `int` and `false` for `bool`
  * `var` IDENTIFIER `=` expression `;`
    * the variable type is inferred from the initializer expression
  * `var` IDENTIFIER `:` type `=` expression `;`
    * the expression is possibly converted implicitly
* assignments
  * `expr`<sub>target</sub> = `expr`<sub>value</sub> `;`
    * the target must be an L-value
* types
  * `int` : 64 bit signed integer
  * `bool` : boolean value, `true` or `false`
* expressions
  * decimal integer literal : `[0-9]+`
  * boolean literal : `true` or `false`
  * variables
    * must be used not before the end of their own declaration
* conversion
  * `int` to `bool` : x = 0 => `false`, otherwise `true`
  * `bool` to `int` : x = `false` => 0, x = `true` => 1