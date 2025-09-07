#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <windows.h>
using namespace std;

// JVM data types
using jbyte = int8_t;
using jshort = int16_t;
using jint = int32_t;
using jlong = int64_t;
using jfloat = float;
using jdouble = double;

// Object system
struct Object;
struct Class;
struct Method;
struct Field;

using ObjectPtr = shared_ptr<Object>;
using ClassPtr = shared_ptr<Class>;
using MethodPtr = shared_ptr<Method>;

// Forward declarations
struct JVMInstance;
struct Frame;
struct MemoryFile {
    vector<uint8_t> data;
    vector<uint8_t> key; // key array
    size_t pos = 0;

    // //////
    MemoryFile(const vector<uint8_t>& d, const uint8_t* k, size_t ksize)
        : data(d), key(k, k + ksize) {}

    uint8_t read_u1() {
        if (pos >= data.size()) throw runtime_error("End of memory");
        uint8_t b = data[pos] ^ key[pos % key.size()]; // 
        pos++;
        return b;
    }

    uint16_t read_u2() {
        return (static_cast<uint16_t>(read_u1()) << 8) | read_u1();
    }

    uint32_t read_u4() {
        return (static_cast<uint32_t>(read_u1()) << 24) |
            (static_cast<uint32_t>(read_u1()) << 16) |
            (static_cast<uint32_t>(read_u1()) << 8) |
            read_u1();
    }

    void read_bytes(char* buffer, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buffer[i] = read_u1();
        }
    }

    void seek(size_t p) { pos = p; }
    size_t tell() const { return pos; }
};




// Constant pool entry types
struct CPEntry {
    uint8_t tag;
    string utf8_value;
    uint16_t class_index;
    uint16_t name_and_type_index;
    uint16_t string_index;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint32_t int_value;
    
    CPEntry() : tag(0), class_index(0), name_and_type_index(0), 
                string_index(0), name_index(0), descriptor_index(0), int_value(0) {}
};

struct Field {
    string name;
    string descriptor;
    bool isStatic = false;
    ObjectPtr refValue;
    jint intValue = 0;

    Field() = default;
    Field(const Field& other) : name(other.name), descriptor(other.descriptor), 
                                isStatic(other.isStatic), refValue(other.refValue), 
                                intValue(other.intValue) {}
    Field& operator=(const Field& other) {
        if (this != &other) {
            name = other.name;
            descriptor = other.descriptor;
            isStatic = other.isStatic;
            refValue = other.refValue;
            intValue = other.intValue;
        }
        return *this;
    }
};

struct Object {
    ClassPtr clazz;
    vector<ObjectPtr> refs;
    vector<jbyte> bytes;
    string stringValue; // For String objects
    
    Object(ClassPtr c) : clazz(c) {}
};

struct Method {
    string name;
    string descriptor;
    vector<uint8_t> code;
    int max_stack = 0;
    int max_locals = 0;
    bool isStatic = false;
    ClassPtr owner;

    Method(ClassPtr cls) : owner(cls) {}
};

struct Class {
    string name;
    ClassPtr superClass;
    vector<Field> fields;
    vector<Method> methods;
    unordered_map<string, int> fieldMap;
    unordered_map<string, int> methodMap;
    vector<CPEntry> constantPool;

    Class(const string& n) : name(n) {}
};

// Stack slot that can hold either int or reference
struct StackSlot {
    enum Type { INT, REF } type;
    jint intValue;
    ObjectPtr refValue;
    
    StackSlot(jint i) : type(INT), intValue(i) {}
    StackSlot(ObjectPtr r) : type(REF), refValue(r), intValue(0) {}
    StackSlot() : type(INT), intValue(0) {}
};

struct Frame {
    Method* method;
    vector<StackSlot> locals;
    stack<StackSlot> operands;
    int pc = 0;

    Frame(Method* m) : method(m) {
        if (m) {
            locals.resize(m->max_locals);
        }
    }
};

struct JVMInstance {
    stack<Frame> callStack;
    unordered_map<string, ClassPtr> loadedClasses;
    ObjectPtr systemOut;

    JVMInstance() {
        bootstrap();
    }

    void bootstrap() {
        auto objClass = make_shared<Class>("java/lang/Object");
        auto strClass = make_shared<Class>("java/lang/String");
        strClass->superClass = objClass;


	// nextLine()
	Method nextLine(scannerClass);
	nextLine.name = "nextLine";
	nextLine.descriptor = "()Ljava/lang/String;";
	nextLine.isStatic = false;
	scannerClass->methods.push_back(nextLine);
	scannerClass->methodMap["nextLine()Ljava/lang/String;"] = 0;

	
	Method nextInt(scannerClass);
	nextInt.name = "nextInt";
	nextInt.descriptor = "()I";
	nextInt.isStatic = false;
	scannerClass->methods.push_back(nextInt);
	scannerClass->methodMap["nextInt()I"] = 1;

	loadedClasses["java/util/Scanner"] = scannerClass;


        // Add String.equals method
        Method equalsMethod(strClass);
        equalsMethod.name = "equals";
        equalsMethod.descriptor = "(Ljava/lang/Object;)Z";
        equalsMethod.isStatic = false;
        strClass->methods.push_back(equalsMethod);
        strClass->methodMap["equals(Ljava/lang/Object;)Z"] = 0;

        auto psClass = make_shared<Class>("java/io/PrintStream");
        psClass->superClass = objClass;

        // Add PrintStream.println(String) method
        Method printlnStrMethod(psClass);
        printlnStrMethod.name = "println";
        printlnStrMethod.descriptor = "(Ljava/lang/String;)V";
        printlnStrMethod.isStatic = false;
        psClass->methods.push_back(printlnStrMethod);
        psClass->methodMap["println(Ljava/lang/String;)V"] = 0;

        // Add PrintStream.println(int) method  
        Method printlnIntMethod(psClass);
        printlnIntMethod.name = "println";
        printlnIntMethod.descriptor = "(I)V";
        printlnIntMethod.isStatic = false;
        psClass->methods.push_back(printlnIntMethod);
        psClass->methodMap["println(I)V"] = 1;

        auto sysClass = make_shared<Class>("java/lang/System");
        sysClass->superClass = objClass;

        Field outField;
        outField.name = "out";
        outField.descriptor = "Ljava/io/PrintStream;";
        outField.isStatic = true;

        auto psObj = make_shared<Object>(psClass);
        outField.refValue = psObj;
        sysClass->fields.push_back(outField);
        sysClass->fieldMap["out"] = 0;

        systemOut = psObj;

        loadedClasses["java/lang/Object"] = objClass;
        loadedClasses["java/lang/String"] = strClass;
        loadedClasses["java/lang/System"] = sysClass;
        loadedClasses["java/io/PrintStream"] = psClass;
    }

    ObjectPtr createString(const string& value) {
        auto strClass = loadedClasses["java/lang/String"];
        auto strObj = make_shared<Object>(strClass);
        strObj->stringValue = value;
        return strObj;
    }


    pair<string, string> resolveMethodRef(const vector<CPEntry>& cp, uint16_t index) {
        if (index >= cp.size() || (cp[index].tag != 10 && cp[index].tag != 11)) {
            return {"", ""};
        }
        
        uint16_t classIndex = cp[index].class_index;
        uint16_t nameAndTypeIndex = cp[index].name_and_type_index;
        
        string className = "";
        if (classIndex < cp.size() && cp[classIndex].tag == 7) {
            uint16_t nameIndex = cp[classIndex].name_index;
            if (nameIndex < cp.size() && cp[nameIndex].tag == 1) {
                className = cp[nameIndex].utf8_value;
            }
        }
        
        string methodName = "", methodDescriptor = "";
        if (nameAndTypeIndex < cp.size() && cp[nameAndTypeIndex].tag == 12) {
            uint16_t nameIdx = cp[nameAndTypeIndex].name_index;
            uint16_t descIdx = cp[nameAndTypeIndex].descriptor_index;
            
            if (nameIdx < cp.size() && cp[nameIdx].tag == 1) {
                methodName = cp[nameIdx].utf8_value;
            }
            if (descIdx < cp.size() && cp[descIdx].tag == 1) {
                methodDescriptor = cp[descIdx].utf8_value;
            }
        }
        
        return {methodName, methodDescriptor};
    }

    ClassPtr loadClassFromFile(const string& filename) {

        ifstream f(filename, ios::binary);
        if (!f) throw runtime_error("Cannot open file: " + filename);
        vector<uint8_t> encrypted((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
        uint8_t key[20] = { 0xAA, 0x3F, 0xC2, 0x7D, 0x91, 0x4B, 0x6E, 0xF0, 0x12, 0x8D,
                            0x55, 0x99, 0x0A, 0xDE, 0x6B, 0x3C, 0x47, 0x81, 0x2F, 0xB4 };


        MemoryFile mem(encrypted, key, sizeof(key));


        uint32_t magic = mem.read_u4();
        if (magic != 0xCAFEBABE) throw runtime_error("Invalid magic number");

        uint16_t minor = mem.read_u2();
        uint16_t major = mem.read_u2();

        uint16_t cp_count = mem.read_u2();
        vector<CPEntry> cp_table(cp_count);

        for (int i = 1; i < cp_count; ++i) {
            uint8_t tag = mem.read_u1();
            cp_table[i].tag = tag;
            switch (tag) {
            case 1: { // UTF8
                uint16_t len = mem.read_u2();
                string s(len, '\0');
                mem.read_bytes(&s[0], len);
                cp_table[i].utf8_value = s;
                break;
            }
            case 3: cp_table[i].int_value = mem.read_u4(); break;
            case 4: mem.read_u4(); break; // float
            case 5: mem.read_u4(); mem.read_u4(); i++; break; // long
            case 6: mem.read_u4(); mem.read_u4(); i++; break; // double
            case 7: cp_table[i].name_index = mem.read_u2(); break;
            case 8: cp_table[i].string_index = mem.read_u2(); break;
            case 9: case 10: case 11:
                cp_table[i].class_index = mem.read_u2();
                cp_table[i].name_and_type_index = mem.read_u2();
                break;
            case 12:
                cp_table[i].name_index = mem.read_u2();
                cp_table[i].descriptor_index = mem.read_u2();
                break;
            default:
                throw runtime_error("Unknown constant pool tag: " + to_string(tag));
            }
        }

        uint16_t access_flags = mem.read_u2();
        uint16_t this_class = mem.read_u2();
        uint16_t super_class = mem.read_u2();

        string className;
        if (this_class > 0 && this_class < cp_count && cp_table[this_class].tag == 7) {
            uint16_t name_index = cp_table[this_class].name_index;
            if (name_index > 0 && name_index < cp_count && cp_table[name_index].tag == 1)
                className = cp_table[name_index].utf8_value;
        }

        if (className.empty()) throw runtime_error("Cannot determine class name");

        if (loadedClasses.count(className)) return loadedClasses[className];

        auto clazz = make_shared<Class>(className);
        loadedClasses[className] = clazz;
        clazz->constantPool = cp_table;

        // Interfaces
        uint16_t interfaces_count = mem.read_u2();
        for (int i = 0; i < interfaces_count; ++i) mem.read_u2();

        // Fields
        uint16_t fields_count = mem.read_u2();
        for (int i = 0; i < fields_count; ++i) {
            Field f;
            uint16_t f_access = mem.read_u2();
            uint16_t f_name = mem.read_u2();
            uint16_t f_desc = mem.read_u2();
            f.isStatic = (f_access & 0x0008) != 0;

            if (f_name > 0 && f_name < cp_count && cp_table[f_name].tag == 1)
                f.name = cp_table[f_name].utf8_value;
            if (f_desc > 0 && f_desc < cp_count && cp_table[f_desc].tag == 1)
                f.descriptor = cp_table[f_desc].utf8_value;

            uint16_t attr_count = mem.read_u2();
            for (int j = 0; j < attr_count; ++j) {
                uint16_t attr_name = mem.read_u2();
                uint32_t attr_len = mem.read_u4();
                mem.seek(mem.tell() + attr_len);
            }

            clazz->fields.push_back(f);
            clazz->fieldMap[f.name] = clazz->fields.size() - 1;
        }

        // Methods
        uint16_t methods_count = mem.read_u2();
        for (int i = 0; i < methods_count; ++i) {
            Method m(clazz);
            uint16_t m_access = mem.read_u2();
            uint16_t m_name = mem.read_u2();
            uint16_t m_desc = mem.read_u2();
            m.isStatic = (m_access & 0x0008) != 0;

            if (m_name > 0 && m_name < cp_count && cp_table[m_name].tag == 1)
                m.name = cp_table[m_name].utf8_value;
            if (m_desc > 0 && m_desc < cp_count && cp_table[m_desc].tag == 1)
                m.descriptor = cp_table[m_desc].utf8_value;

            uint16_t attr_count = mem.read_u2();
            for (int j = 0; j < attr_count; ++j) {
                uint16_t attr_name = mem.read_u2();
                uint32_t attr_len = mem.read_u4();
                string attrName;
                if (attr_name > 0 && attr_name < cp_count && cp_table[attr_name].tag == 1)
                    attrName = cp_table[attr_name].utf8_value;

                if (attrName == "Code") {
                    m.max_stack = mem.read_u2();
                    m.max_locals = mem.read_u2();
                    uint32_t code_length = mem.read_u4();
                    m.code.resize(code_length);
                    mem.read_bytes(reinterpret_cast<char*>(m.code.data()), code_length);

                    uint16_t ex_table_len = mem.read_u2();
                    mem.seek(mem.tell() + ex_table_len * 8);

                    uint16_t code_attr_count = mem.read_u2();
                    for (int k = 0; k < code_attr_count; ++k) {
                        uint16_t ca_name = mem.read_u2();
                        uint32_t ca_len = mem.read_u4();
                        mem.seek(mem.tell() + ca_len);
                    }
                }
                else {
                    mem.seek(mem.tell() + attr_len);
                }
            }

            clazz->methods.push_back(m);
            clazz->methodMap[m.name + m.descriptor] = clazz->methods.size() - 1;
        }

        return clazz;
    }

    void runMain(const string& className) {
        auto it = loadedClasses.find(className);
        if (it == loadedClasses.end()) {
            throw runtime_error("Class not loaded: " + className);
        }

        auto clazz = it->second;
        string mainKey = "main([Ljava/lang/String;)V";

        auto mit = clazz->methodMap.find(mainKey);
        if (mit == clazz->methodMap.end()) {
            throw runtime_error("Main method not found in class " + className);
        }

        auto& method = clazz->methods[mit->second];
        Frame frame(&method);
        callStack.push(frame);

        execute();
    }

    void execute() {
        while (!callStack.empty()) {
            auto& frame = callStack.top();
            if (!frame.method) {
                callStack.pop();
                continue;
            }
            auto& code = frame.method->code;

            if (frame.pc >= (int)code.size()) {
                callStack.pop();
                continue;
            }

            uint8_t opcode = code[frame.pc++];
            executeOpcode(frame, code, opcode);
        }
    }

    void executeOpcode(Frame& frame, const vector<uint8_t>& code, uint8_t opcode) {
        auto& locals = frame.locals;
        auto& operands = frame.operands;

        switch (opcode) {
            case 0x00: break; // nop

            case 0x01: operands.push(StackSlot(ObjectPtr(nullptr))); break; // aconst_null
            case 0x02: operands.push(StackSlot(-1)); break; // iconst_m1
            case 0x03: operands.push(StackSlot(0)); break;  // iconst_0
            case 0x04: operands.push(StackSlot(1)); break;  // iconst_1
            case 0x05: operands.push(StackSlot(2)); break;  // iconst_2
            case 0x06: operands.push(StackSlot(3)); break;  // iconst_3
            case 0x07: operands.push(StackSlot(4)); break;  // iconst_4
            case 0x08: operands.push(StackSlot(5)); break;  // iconst_5

            case 0x10: { // bipush
                jbyte val = static_cast<jbyte>(code[frame.pc++]);
                operands.push(StackSlot(static_cast<jint>(val)));
                break;
            }
            case 0x11: { // sipush
                uint16_t raw_val = (static_cast<uint16_t>(code[frame.pc]) << 8) | 
                                   static_cast<uint16_t>(code[frame.pc + 1]);
                frame.pc += 2;
                jshort val = static_cast<jshort>(raw_val);
                operands.push(StackSlot(static_cast<jint>(val)));
                break;
            }

            case 0x12: { // ldc
                uint8_t index = code[frame.pc++];
                if (index < frame.method->owner->constantPool.size()) {
                    auto& entry = frame.method->owner->constantPool[index];
                    if (entry.tag == 8) { // String constant
                        uint16_t utf8_index = entry.string_index;
                        if (utf8_index < frame.method->owner->constantPool.size() && 
                            frame.method->owner->constantPool[utf8_index].tag == 1) {
                            string strValue = frame.method->owner->constantPool[utf8_index].utf8_value;
                            auto strObj = createString(strValue);
                            operands.push(StackSlot(strObj));
                        }
                    } else if (entry.tag == 3) { // Integer constant
                        operands.push(StackSlot(static_cast<jint>(entry.int_value)));
                    }
                }
                break;
            }
            
            case 0x13: { // ldc_w
                uint16_t index = (static_cast<uint16_t>(code[frame.pc]) << 8) | 
                                static_cast<uint16_t>(code[frame.pc + 1]);
                frame.pc += 2;
                if (index < frame.method->owner->constantPool.size()) {
                    auto& entry = frame.method->owner->constantPool[index];
                    if (entry.tag == 8) { // String constant
                        uint16_t utf8_index = entry.string_index;
                        if (utf8_index < frame.method->owner->constantPool.size() && 
                            frame.method->owner->constantPool[utf8_index].tag == 1) {
                            string strValue = frame.method->owner->constantPool[utf8_index].utf8_value;
                            auto strObj = createString(strValue);
                            operands.push(StackSlot(strObj));
                        }
                    } else if (entry.tag == 3) { // Integer constant
                        operands.push(StackSlot(static_cast<jint>(entry.int_value)));
                    }
                }
                break;
            }

            case 0x15: { // iload
                uint8_t idx = code[frame.pc++];
                if (idx < locals.size()) {
                    operands.push(locals[idx]);
                }
                break;
            }
            case 0x1A: if (locals.size() > 0) operands.push(locals[0]); break; // iload_0
            case 0x1B: if (locals.size() > 1) operands.push(locals[1]); break; // iload_1
            case 0x1C: if (locals.size() > 2) operands.push(locals[2]); break; // iload_2
            case 0x1D: if (locals.size() > 3) operands.push(locals[3]); break; // iload_3

            case 0x19: { // aload
                uint8_t idx = code[frame.pc++];
                if (idx < locals.size()) {
                    operands.push(locals[idx]);
                }
                break;
            }
            case 0x2A: if (locals.size() > 0) operands.push(locals[0]); break; // aload_0
            case 0x2B: if (locals.size() > 1) operands.push(locals[1]); break; // aload_1
            case 0x2C: if (locals.size() > 2) operands.push(locals[2]); break; // aload_2
            case 0x2D: if (locals.size() > 3) operands.push(locals[3]); break; // aload_3

            case 0x36: { // istore
                uint8_t idx = code[frame.pc++];
                if (!operands.empty() && idx < locals.size()) {
                    locals[idx] = operands.top(); 
                    operands.pop();
                }
                break;
            }
            case 0x3B: if (!operands.empty() && locals.size() > 0) { locals[0] = operands.top(); operands.pop(); } break; // istore_0
            case 0x3C: if (!operands.empty() && locals.size() > 1) { locals[1] = operands.top(); operands.pop(); } break; // istore_1
            case 0x3D: if (!operands.empty() && locals.size() > 2) { locals[2] = operands.top(); operands.pop(); } break; // istore_2
            case 0x3E: if (!operands.empty() && locals.size() > 3) { locals[3] = operands.top(); operands.pop(); } break; // istore_3

            case 0x3A: { // astore
                uint8_t idx = code[frame.pc++];
                if (!operands.empty() && idx < locals.size()) {
                    locals[idx] = operands.top(); 
                    operands.pop();
                }
                break;
            }
            case 0x4B: if (!operands.empty() && locals.size() > 0) { locals[0] = operands.top(); operands.pop(); } break; // astore_0
            case 0x4C: if (!operands.empty() && locals.size() > 1) { locals[1] = operands.top(); operands.pop(); } break; // astore_1
            case 0x4D: if (!operands.empty() && locals.size() > 2) { locals[2] = operands.top(); operands.pop(); } break; // astore_2
            case 0x4E: if (!operands.empty() && locals.size() > 3) { locals[3] = operands.top(); operands.pop(); } break; // astore_3

            case 0x57: if (!operands.empty()) operands.pop(); break; // pop
            case 0x59: { // dup
                if (!operands.empty()) {
                    auto v = operands.top();
                    operands.push(v);
                }
                break;
            }

            case 0x60: { // iadd
                if (operands.size() >= 2) {
                    auto b = operands.top(); operands.pop();
                    auto a = operands.top(); operands.pop();
                    if (a.type == StackSlot::INT && b.type == StackSlot::INT) {
                        operands.push(StackSlot(a.intValue + b.intValue));
                    }
                }
                break;
            }
            case 0x64: { // isub
                if (operands.size() >= 2) {
                    auto b = operands.top(); operands.pop();
                    auto a = operands.top(); operands.pop();
                    if (a.type == StackSlot::INT && b.type == StackSlot::INT) {
                        operands.push(StackSlot(a.intValue - b.intValue));
                    }
                }
                break;
            }
            case 0x68: { // imul
                if (operands.size() >= 2) {
                    auto b = operands.top(); operands.pop();
                    auto a = operands.top(); operands.pop();
                    if (a.type == StackSlot::INT && b.type == StackSlot::INT) {
                        operands.push(StackSlot(a.intValue * b.intValue));
                    }
                }
                break;
            }
            



            case 0x6C: { // idiv
                if (operands.size() >= 2) {
                    auto b = operands.top(); operands.pop();
                    auto a = operands.top(); operands.pop();
                    if (a.type == StackSlot::INT && b.type == StackSlot::INT) {
                        if (b.intValue == 0) throw runtime_error("Division by zero");
                        operands.push(StackSlot(a.intValue / b.intValue));
                    }
                }
                break;
            }


            case 0x0e: {
                operands.push(StackSlot(ObjectPtr(nullptr)));
                break; 
            }
                



            case 0x84: { // iinc
                uint8_t idx = code[frame.pc++];
                jbyte increment = static_cast<jbyte>(code[frame.pc++]);
                if (idx < locals.size() && locals[idx].type == StackSlot::INT) {
                    locals[idx].intValue += increment;
                }
                break;
            }

            case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: { // ifeq, ifne, etc.
                if (!operands.empty()) {
                    auto slot = operands.top(); operands.pop();
                    if (slot.type == StackSlot::INT) {
                        jint val = slot.intValue;
                        uint16_t raw_offset = (static_cast<uint16_t>(code[frame.pc]) << 8) | 
                                             static_cast<uint16_t>(code[frame.pc + 1]);
                        jshort offset = static_cast<jshort>(raw_offset);
                        frame.pc += 2;

                        bool jump = false;
                        switch (opcode) {
                            case 0x99: jump = (val == 0); break; // ifeq
                            case 0x9A: jump = (val != 0); break; // ifne
                            case 0x9B: jump = (val < 0); break;  // iflt
                            case 0x9C: jump = (val >= 0); break; // ifge
                            case 0x9D: jump = (val > 0); break;  // ifgt
                            case 0x9E: jump = (val <= 0); break; // ifle
                        }
                        if (jump) frame.pc = frame.pc - 3 + offset;
                    }
                }
                break;
            }

            case 0x9F: case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: { // if_icmpeq, if_icmpne, etc.
                if (operands.size() >= 2) {
                    auto slot2 = operands.top(); operands.pop();
                    auto slot1 = operands.top(); operands.pop();
                    if (slot1.type == StackSlot::INT && slot2.type == StackSlot::INT) {
                        jint val1 = slot1.intValue;
                        jint val2 = slot2.intValue;
                        uint16_t raw_offset = (static_cast<uint16_t>(code[frame.pc]) << 8) | 
                                             static_cast<uint16_t>(code[frame.pc + 1]);
                        jshort offset = static_cast<jshort>(raw_offset);
                        frame.pc += 2;

                        bool jump = false;
                        switch (opcode) {
                            case 0x9F: jump = (val1 == val2); break; // if_icmpeq
                            case 0xA0: jump = (val1 != val2); break; // if_icmpne
                            case 0xA1: jump = (val1 < val2); break;  // if_icmplt
                            case 0xA2: jump = (val1 >= val2); break; // if_icmpge
                            case 0xA3: jump = (val1 > val2); break;  // if_icmpgt
                            case 0xA4: jump = (val1 <= val2); break; // if_icmple
                        }
                        if (jump) frame.pc = frame.pc - 3 + offset;
                    }
                }
                break;
            }


            case 0xA5: case 0xA6: { // if_acmpeq, if_acmpne
                if (operands.size() >= 2) {
                    auto slot2 = operands.top(); operands.pop();
                    auto slot1 = operands.top(); operands.pop();
                    if (slot1.type == StackSlot::REF && slot2.type == StackSlot::REF) {
                        ObjectPtr ref1 = slot1.refValue;
                        ObjectPtr ref2 = slot2.refValue;
                        uint16_t raw_offset = (static_cast<uint16_t>(code[frame.pc]) << 8) | 
                                             static_cast<uint16_t>(code[frame.pc + 1]);
                        jshort offset = static_cast<jshort>(raw_offset);
                        frame.pc += 2;

                        bool jump = false;
                        switch (opcode) {
                            case 0xA5: jump = (ref1 == ref2); break; // if_acmpeq
                            case 0xA6: jump = (ref1 != ref2); break; // if_acmpne
                        }
                        if (jump) frame.pc = frame.pc - 3 + offset;
                    }
                }
                break;
            }

            case 0xA7: { // goto
                uint16_t raw_offset = (static_cast<uint16_t>(code[frame.pc]) << 8) | 
                                     static_cast<uint16_t>(code[frame.pc + 1]);
                jshort offset = static_cast<jshort>(raw_offset);
                frame.pc = frame.pc - 1 + offset;
                break;
            }

            case 0xB2: { // getstatic
                uint16_t index = (static_cast<uint16_t>(code[frame.pc]) << 8) | 
                                static_cast<uint16_t>(code[frame.pc + 1]);
                frame.pc += 2;
                operands.push(StackSlot(systemOut));
                break;
            }
            case 0xB8: { // invokestatic
                uint16_t index = (static_cast<uint16_t>(code[frame.pc]) << 8) |
                    static_cast<uint16_t>(code[frame.pc + 1]);
                frame.pc += 2;

                auto [methodName, methodDescriptor] = resolveMethodRef(frame.method->owner->constantPool, index);


                if (methodName == "input" && methodDescriptor == "(Ljava/lang/String;)Ljava/lang/String;") {

                    if (!frame.operands.empty()) {
                        auto promptSlot = frame.operands.top(); frame.operands.pop();
                        string promptText;
                        if (promptSlot.type == StackSlot::REF && promptSlot.refValue &&
                            promptSlot.refValue->clazz->name == "java/lang/String") {
                            promptText = promptSlot.refValue->stringValue;
                        }


                        cout << promptText;
                        string inputLine;
                        getline(cin, inputLine);

                        // string to Stack
                        auto strObj = createString(inputLine);
                        frame.operands.push(StackSlot(strObj));
                    }
                    break;
                }

                // you can add other static methods 
                break;
            }

            case 0xB6: { // invokevirtual
                uint16_t index = (static_cast<uint16_t>(code[frame.pc]) << 8) |
                    static_cast<uint16_t>(code[frame.pc + 1]);
                frame.pc += 2;

                auto [methodName, methodDescriptor] = resolveMethodRef(frame.method->owner->constantPool, index);


                // println(String)
                if (methodName == "println" && methodDescriptor == "(Ljava/lang/String;)V") {
                    if (operands.size() >= 2) {
                        auto argSlot = operands.top(); operands.pop();
                        auto objSlot = operands.top(); operands.pop();
                        if (argSlot.type == StackSlot::REF && argSlot.refValue &&
                            argSlot.refValue->clazz->name == "java/lang/String") {
                            cout << argSlot.refValue->stringValue << endl;
                        }
                    }
                    break;
                }

                // println(int)
                if (methodName == "println" && methodDescriptor == "(I)V") {
                    if (operands.size() >= 2) {
                        auto argSlot = operands.top(); operands.pop();
                        auto objSlot = operands.top(); operands.pop();
                        if (argSlot.type == StackSlot::INT) {
                            cout << argSlot.intValue << endl;
                        }
                    }
                    break;
                }

                if (methodName == "equals" && methodDescriptor == "(Ljava/lang/Object;)Z") {
                    auto argSlot = operands.top(); operands.pop();
                    auto objSlot = operands.top(); operands.pop();

                    bool result = false;
                    if (objSlot.type == StackSlot::REF && argSlot.type == StackSlot::REF &&
                        objSlot.refValue && argSlot.refValue) {
                        result = (objSlot.refValue->stringValue == argSlot.refValue->stringValue);
                    }
                    operands.push(StackSlot(result ? 1 : 0));
                    break;
                }
                break;
            }




            case 0xB1: // return
                callStack.pop();
                return;

            default:
                cerr << "Unimplemented opcode: 0x" << hex << setfill('0') << setw(2) << (int)opcode << dec << endl;
                break;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <classfile.class>" << endl;
        return 1;
    }

    try {
        JVMInstance jvm;
        string filename = argv[1];
        
        
        cout << "Starting JVM...\n";
        Sleep(200); // sleep JIC
        // load class
        ClassPtr clazz = jvm.loadClassFromFile(filename);
        string className = clazz->name;
        
        
        jvm.runMain(className);
        Sleep(200);
        cout << "JVM has been executed";
    } catch (const exception& e) {
        cerr << "err: " << e.what() << endl;
        return 1;
    }

    return 0;
}