# 🟦 MiniJVM (Custom Java Virtual Machine in C++)

This is a minimal **Java Virtual Machine** implementation written in C++.  
It can load `.class` files (with simple XOR encryption), parse the constant pool, execute bytecode, and emulate part of the standard Java classes (`System.out`, `String.equals`, `PrintStream.println`, etc.).

⚡ Features:
- Load `.class` files with **xor-based encryption**.  
- Parse and store **constant pool**.  
- Support for a subset of **JVM bytecodes**:  
  `iload`, `istore`, `iadd`, `isub`, `imul`, `idiv`, `if_icmpXX`, `goto`, `invokevirtual`, `invokestatic`, `return`, and more.  
- Minimal object model: `Object`, `Class`, `Field`, `Method`.  
- Built-in stubs for standard Java classes:  
  - `java/lang/String`  
  - `java/io/PrintStream` (`println`)  
  - `java/lang/System` (`System.out`)  
- Console input/output support (`input()`, `println()`).

---

## 🚀 Build

### Windows (MSVC)

g++ -std=c++17 -O2 -o jvm jvm.cpp
