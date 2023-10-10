#include <iostream>
#include <string>
#include <map>
#include <vector>

using namespace std;

unsigned int extractBits(unsigned int number, int k, int p) {
    return (((1 << k) - 1) & (number >> (p - 1)));
}

string decodeRFormat(unsigned int instr) {
    map<pair<unsigned int, unsigned int>, string> functs = {
        {{0x00, 0x33}, "add"},
        {{0x20, 0x33}, "sub"},
        {{0x07, 0x33}, "and"},
        {{0x06, 0x33}, "or"},
        {{0x04, 0x33}, "xor"},
        {{0x01, 0x33}, "sll"},
        {{0x05, 0x33}, "srl"},
        {{0x25, 0x33}, "sra"}
    };

    unsigned int opcode = extractBits(instr, 7, 1);
    unsigned int rd = extractBits(instr, 5, 8);
    unsigned int funct3 = extractBits(instr, 3, 13);
    unsigned int rs1 = extractBits(instr, 5, 16);
    unsigned int rs2 = extractBits(instr, 5, 21);
    unsigned int funct7 = extractBits(instr, 7, 26);

    string operation = functs[{funct7, opcode}];

    return operation + " x" + to_string(rd) + ", x" + to_string(rs1) + ", x" + to_string(rs2);
}

string decodeIFormat(unsigned int instr) {
    map<unsigned int, string> functs = {
        {0x13, "addi"},
        {0x1B, "andi"},
        {0x17, "ori"},
        {0x0F, "xori"},
        {0x03, "slli"},
        {0x07, "srli"},
        {0x27, "srai"}
    };

    unsigned int opcode = extractBits(instr, 7, 1);
    unsigned int rd = extractBits(instr, 5, 8);
    unsigned int funct3 = extractBits(instr, 3, 13);
    unsigned int rs1 = extractBits(instr, 5, 16);
    int imm = extractBits(instr, 12, 21); // Signed value

    if (imm & (1 << 11)) {
        imm |= 0xFFFFF000;
    }

    string operation = functs[opcode];

    return operation + " x" + to_string(rd) + ", x" + to_string(rs1) + ", " + to_string(imm);
}

string decodeSFormat(unsigned int instr) {
    map<unsigned int, string> functs = {
        {0x23, "sd"},
        {0x2B, "sw"},
        {0x1F, "sh"},
        {0x0B, "sb"}
    };

    unsigned int opcode = extractBits(instr, 7, 1);
    unsigned int funct3 = extractBits(instr, 3, 13);
    unsigned int rs1 = extractBits(instr, 5, 16);
    unsigned int rs2 = extractBits(instr, 5, 21);
    unsigned int imm = (extractBits(instr, 7, 26) << 5) | extractBits(instr, 5, 8);

    string operation = functs[opcode];

    return operation + " x" + to_string(rs2) + ", " + to_string(imm) + "(x" + to_string(rs1) + ")";
}

map<int, string> labelMap;

void firstPass(vector<unsigned int>& instructions) {
    int pc = 0;
    int labelCount = 1;

    for (unsigned int instr : instructions) {
        unsigned int opcode = extractBits(instr, 7, 1);

        if (opcode == 0x63 || opcode == 0x6F) {
            int offset;
            if (opcode == 0x63) { // B-format
                offset = (extractBits(instr, 1, 8) << 11) | (extractBits(instr, 6, 26) << 5) |
                         (extractBits(instr, 4, 9) << 1) | (extractBits(instr, 1, 32) << 12);
                if (offset & (1 << 12)) {
                    offset |= 0xFFFFE000;
                }
            } else { // J-format
                offset = (extractBits(instr, 1, 21) << 20) | (extractBits(instr, 10, 13) << 1) |
                         (extractBits(instr, 1, 32) << 11) | (extractBits(instr, 10, 22) << 12);
                if (offset & (1 << 20)) {
                    offset |= 0xFFE00000;
                }
            }
            int targetAddress = pc + offset;
            if (labelMap.find(targetAddress) == labelMap.end()) {
                labelMap[targetAddress] = "L" + to_string(labelCount++);
            }
        }
        pc += 4;
    }
}

string decodeBFormat(unsigned int instr, int pc) {
    map<unsigned int, string> functs = {
        {0x63, "beq"},
        {0x6B, "bne"},
        {0x3F, "blt"},
        {0x2F, "bge"},
        {0x1D, "bltu"},
        {0x0D, "bgeu"}
    };

    unsigned int opcode = extractBits(instr, 7, 1);
    unsigned int funct3 = extractBits(instr, 3, 13);
    unsigned int rs1 = extractBits(instr, 5, 16);
    unsigned int rs2 = extractBits(instr, 5, 21);
    unsigned int imm = (extractBits(instr, 1, 8) << 11) | (extractBits(instr, 6, 26) << 5) | 
                       (extractBits(instr, 4, 9) << 1) | (extractBits(instr, 1, 32) << 12);

    if (imm & (1 << 12)) {
        imm |= 0xFFFFE000;
    }

    string operation = functs[opcode];

    int targetAddress = pc + imm;
    return operation + " x" + to_string(rs1) + ", x" + to_string(rs2) + ", " + labelMap[targetAddress];
}

string decodeJFormat(unsigned int instr, int pc) {
    unsigned int opcode = extractBits(instr, 7, 1);
    unsigned int rd = extractBits(instr, 5, 8);
    unsigned int imm = (extractBits(instr, 1, 21) << 20) | (extractBits(instr, 10, 13) << 1) |
                       (extractBits(instr, 1, 32) << 11) | (extractBits(instr, 10, 22) << 12);

    if (imm & (1 << 20)) {
        imm |= 0xFFE00000;
    }

    int targetAddress = pc + imm;
    return "jal x" + to_string(rd) + ", " + labelMap[targetAddress];
}

string decodeUFormat(unsigned int instr) {
    unsigned int opcode = extractBits(instr, 7, 1);
    unsigned int rd = extractBits(instr, 5, 8);
    unsigned int imm = extractBits(instr, 20, 13) << 12;

    return "lui x" + to_string(rd) + ", 0x" + to_string(imm);
}

int main() {
    vector<unsigned int> instructions = {0x007201b3, 0x00c50493, 0x00533623, 0x00720863, 0x00c0006f, 0x100004b7};

    firstPass(instructions);

    int pc = 0;
    for (unsigned int instr : instructions) {
        if (labelMap.find(pc) != labelMap.end()) {
            cout << labelMap[pc] << ": ";
        }

        unsigned int opcode = extractBits(instr, 7, 1);

        if (opcode == 0x33) {
            cout << decodeRFormat(instr) << endl;
        } else if (opcode == 0x13 || opcode == 0x03 || opcode == 0x07 || opcode == 0x1B || opcode == 0x17 || opcode == 0x0F || opcode == 0x27) {
            cout << decodeIFormat(instr) << endl;
        } else if (opcode == 0x23 || opcode == 0x2B || opcode == 0x1F || opcode == 0x0B) {
            cout << decodeSFormat(instr) << endl;
        } else if (opcode == 0x63) {
            cout << decodeBFormat(instr, pc) << endl;
        } else if (opcode == 0x6F) {
            cout << decodeJFormat(instr, pc) << endl;
        } else if (opcode == 0x37) {
            cout << decodeUFormat(instr) << endl;
        } else {
            cout << "Unknown instruction" << endl;
        }

        pc += 4;
    }

    return 0;
}

