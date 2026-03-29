# Ken: Simple Programming Language

**Ken Programming Language** transpile to portable C11 code. The transpiler is written in C and is currently in early development. **Do not use this in production**.

The purpose of creating **Ken Programming Language** is to code simple but **fast like C** & **Safe like rust (probably)**.

## Example

### Hello World

```
func main() -> void {
    let a = "Hello, World!";
    println!(a);
}
```

### String Interpolation

```
func greet(name: str) -> void {
    println!("Hello {name}!");
}
```

## Status

Work in progress. Things will break.
