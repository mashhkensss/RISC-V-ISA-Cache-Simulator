#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <map>
#include <algorithm>
#include <memory>
#include "Parameters/CacheReplacementPolicies.cpp"
#include "Cache/CacheLRU.cpp"
#include "Cache/CachePLRU.cpp"


class CacheSimulator {
public:
    CacheBase* cache;
    uint32_t overallRequests = 0;
    uint32_t Hits = 0;
    explicit CacheSimulator(CacheBase *cache) : cache(cache), Hits(0) {};
    void request(Address address, Type type, std::vector<int8_t>& memory) {
        if (cache->accessMemory(address, type, memory)) ++Hits;
        ++overallRequests;
    }
    [[nodiscard]] double hitRate() const {
        return static_cast<double>(Hits) / overallRequests * 100;
    }
};

class Simulation {
public:
    std::vector<CacheSimulator> simulators;
    std::vector<int32_t> registers;
    ReplacementPolicy policy_;
    Simulation(std::vector<CacheSimulator> simulators, ReplacementPolicy policy) : simulators(std::move(simulators)),
                                                                                   policy_(policy), registers(32) {};
    void request(Address address, Type type, std::vector<int8_t>& memory) {
        if (policy_ == ReplacementPolicy(LRU) || policy_ == ReplacementPolicy(ALL))
            simulators[0].request(address, type, memory);
        if (policy_ == ReplacementPolicy(PLRU) || policy_ == ReplacementPolicy(ALL))
            simulators[1].request(address, type, memory);
    }
    int32_t getReg(int x) {
        return registers[x];
    }
    void setReg(int32_t x, int32_t data) {
        if (x < 0 || x >= 32) {
            std::cerr << "Register index out of bounds: " << x << std::endl;
            return;
        }
        if (x == 0) return;
        registers[x] = data;
    }
    double getHitRate(ReplacementPolicy policy) {
        if (policy == ReplacementPolicy(LRU)) return simulators[0].hitRate();
        else return simulators[1].hitRate();
    }
    void printResult() {
        if (policy_ == ReplacementPolicy(ALL) || policy_ == ReplacementPolicy(LRU)) {
            std::printf("LRU\thit rate: %3.4f%%\n", getHitRate(LRU));
        }
        if (policy_ == ReplacementPolicy(ALL) || policy_ == ReplacementPolicy(PLRU)) {
            std::printf("pLRU\thit rate: %3.4f%%\n", getHitRate(PLRU));
        }
    }
};

Address decodeAddress(uint32_t address) {
    uint16_t tag = (address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN)) & ((1 << CACHE_TAG_LEN) - 1);
    uint8_t index = (address >> CACHE_OFFSET_LEN) & ((1 << CACHE_INDEX_LEN) - 1);
    uint8_t offset = address & ((1 << CACHE_OFFSET_LEN) - 1);
    return {tag, index, offset};
}



/*----------------------------- некоторые необходимые значения -------------------------------------------------------*/
std::map<std::string, int> reg_map = {
        {"zero", 0}, {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4}, {"t0", 5}, {"t1", 6}, {"t2", 7},
        {"s0", 8}, {"fp", 8}, {"s1", 9}, {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14}, {"a5", 15},
        {"a6", 16}, {"a7", 17}, {"s2", 18}, {"s3", 19}, {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23},
        {"s8", 24}, {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28}, {"t4", 29}, {"t5", 30}, {"t6", 31},
        {"x0", 0}, {"x1", 1}, {"x2", 2}, {"x3", 3}, {"x4", 4}, {"x5", 5}, {"x6", 6}, {"x7", 7},
        {"x8", 8}, {"x9", 9}, {"x10", 10}, {"x11", 11}, {"x12", 12}, {"x13", 13}, {"x14", 14}, {"x15", 15},
        {"x16", 16}, {"x17", 17}, {"x18", 18}, {"x19", 19}, {"x20", 20}, {"x21", 21}, {"x22", 22}, {"x23", 23},
        {"x24", 24}, {"x25", 25}, {"x26", 26}, {"x27", 27}, {"x28", 28}, {"x29", 29}, {"x30", 30}, {"x31", 31}
};


enum InstrType {
    R, I, S, B, U, J
};

const std::unordered_map<std::string, InstrType> instrMap = {
        // R
        {"add",    R}, {"sub",    R}, {"sll",    R}, {"slt",    R}, {"sltu",   R}, {"xor",    R}, {"srl",    R},
        {"sra",    R}, {"or",     R}, {"and",    R}, {"mul",    R}, {"mulh",   R}, {"mulhsu", R}, {"mulhu",  R},
        {"div",    R}, {"divu",   R}, {"rem",    R}, {"remu",   R},
        // I
        {"addi",   I}, {"lw",     I}, {"slli",   I}, {"slti",   I}, {"sltiu",  I}, {"xori",   I}, {"ori",    I},
        {"andi",   I}, {"lb",     I}, {"lh",     I}, {"ret",    I}, {"li",     I}, {"jalr",    I}, {"srli", I},
        {"lbu",    I}, {"lhu",    I}, {"srai",   I},
        // S
        {"sw",     S}, {"sb",     S}, {"sh",     S},
        // B
        {"beq",    B}, {"bne",    B}, {"blt",    B}, {"bge",    B}, {"bltu",   B}, {"bgeu",   B},
        // U
        {"la",     U}, {"lui",    U}, {"auipc",  U},
        // J
        {"jal",    J}
};


/*----------------------------- некоторые необходимые значения -------------------------------------------------------*/
std::vector<int8_t> memory(262144, 0); // bytes
int pc = 0;

// ФУНКЦИЯ ДЛЯ ПАРСИНГА ДЕСЯТИЧНЫХ И ШЕСТНАДЦАТИРИЧНЫХ ЗНАЧЕНИЙ
int64_t parseImmediate(const std::string& imm) {
    try {
        if (imm.empty()) {
            return 0;
        } else if (imm.find("0x") == 0 || imm.find("0X") == 0) {
            return std::stoull(imm, nullptr, 16);
        } else if (imm.find("-0x") == 0 || imm.find("-0X") == 0) {
            return std::stoull(imm, nullptr, 16);
        } else if (std::isdigit(imm[0]) || imm[0] == '-') {
            return std::stoull(imm);
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid immediate value: " << imm << std::endl;
        return 0;
    } catch (const std::out_of_range& e) {
        std::cerr << "Immediate value out of range: " << imm << std::endl;
        return 0;
    }
}



/*----------------------------- перевод ассемблера в машинный код -----------------------------------------------------*/
uint32_t AssemblyToMachineCode(const std::string& mnemonic, const std::vector<std::string>& args, int address) {
    uint32_t machineCode = 0;
    auto instr = instrMap.find(mnemonic);
    if (instr == instrMap.end()) {
        std::cerr << "Unrecognized instruction mnemonic: " << mnemonic << std::endl;
        return machineCode;
    }

    if (instr->second == R) {
        int rd = reg_map[args[0]];
        int rs1 = reg_map[args[1]];
        int rs2 = reg_map[args[2]];

        int funct3, funct7, opcode;
        if (mnemonic == "add" || mnemonic == "sub" || mnemonic == "mul") {
            opcode = 0x33;
            funct3 = (mnemonic == "mul") ? 0 : 0;
            funct7 = (mnemonic == "sub") ? 0x20 : 0x00;
            funct7 = (mnemonic == "mul") ? 0x01 : funct7;
        } else if (mnemonic == "sll") {
            opcode = 0x33;
            funct3 = 1;
            funct7 = 0x00;
        } else if (mnemonic == "slt") {
            opcode = 0x33;
            funct3 = 2;
            funct7 = 0x00;
        } else if (mnemonic == "sltu") {
            opcode = 0x33;
            funct3 = 3;
            funct7 = 0x00;
        } else if (mnemonic == "xor") {
            opcode = 0x33;
            funct3 = 4;
            funct7 = 0x00;
        } else if (mnemonic == "srl" || mnemonic == "sra") {
            opcode = 0x33;
            funct3 = 5;
            funct7 = (mnemonic == "sra") ? 0x20 : 0x00;
        } else if (mnemonic == "or") {
            opcode = 0x33;
            funct3 = 6;
            funct7 = 0x00;
        } else if (mnemonic == "and") {
            opcode = 0x33;
            funct3 = 7;
            funct7 = 0x00;
        } else if (mnemonic == "mulh") {
            opcode = 0x33;
            funct3 = 1;
            funct7 = 0x01;
        } else if (mnemonic == "mulhsu") {
            opcode = 0x33;
            funct3 = 2;
            funct7 = 0x01;
        } else if (mnemonic == "mulhu") {
            opcode = 0x33;
            funct3 = 3;
            funct7 = 0x01;
        } else if (mnemonic == "div") {
            opcode = 0x33;
            funct3 = 4;
            funct7 = 0x01;
        } else if (mnemonic == "divu") {
            opcode = 0x33;
            funct3 = 5;
            funct7 = 0x01;
        } else if (mnemonic == "rem") {
            opcode = 0x33;
            funct3 = 6;
            funct7 = 0x01;
        } else if (mnemonic == "remu") {
            opcode = 0x33;
            funct3 = 7;
            funct7 = 0x01;
        } else {
            return machineCode;
        }
        machineCode |= (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    } else if (instr->second == I) {
        int rd, rs1;
        int64_t imm;
        int funct3, opcode;

        rd = reg_map[args[0]];
        rs1 = reg_map[args[1]];
        imm = parseImmediate(args[2]);

        imm = imm & 0xFFF;

        if (mnemonic == "addi") {
            opcode = 0x13;
            funct3 = 0;
        } else if (mnemonic == "slti") {
            opcode = 0x13;
            funct3 = 2;
        } else if (mnemonic == "sltiu") {
            opcode = 0x13;
            funct3 = 3;
        } else if (mnemonic == "xori") {
            opcode = 0x13;
            funct3 = 4;
        } else if (mnemonic == "ori") {
            opcode = 0x13;
            funct3 = 6;
        } else if (mnemonic == "andi") {
            opcode = 0x13;
            funct3 = 7;
        } else if (mnemonic == "slli") {
            opcode = 0x13;
            funct3 = 1;
            imm &= 0x1F;
        } else if (mnemonic == "srli" || mnemonic == "srai") {
            opcode = 0x13;
            funct3 = 5;
            if (mnemonic == "srai") {
                imm |= 0x400;
            }
        } else if (mnemonic == "jalr") {
            opcode = 0x67;
            funct3 = 0;
        } else if (mnemonic == "lb") {
            rd = reg_map[args[0]];
            rs1 = reg_map[args[2]];
            imm = parseImmediate(args[1]);
            opcode = 0x03;
            funct3 = 0;
        } else if (mnemonic == "lh") {
            rd = reg_map[args[0]];
            rs1 = reg_map[args[2]];
            imm = parseImmediate(args[1]);
            opcode = 0x03;
            funct3 = 1;
        } else if (mnemonic == "lw") {
            rd = reg_map[args[0]];
            rs1 = reg_map[args[2]];
            imm = parseImmediate(args[1]);
            opcode = 0x03;
            funct3 = 2;
        } else if (mnemonic == "lbu") {
            rd = reg_map[args[0]];
            rs1 = reg_map[args[2]];
            imm = parseImmediate(args[1]);
            opcode = 0x03;
            funct3 = 4;
        } else if (mnemonic == "lhu") {
            rd = reg_map[args[0]];
            rs1 = reg_map[args[2]];
            imm = parseImmediate(args[1]);
            opcode = 0x03;
            funct3 = 5;
        } else {
            return machineCode;
        }

        machineCode |= ((imm & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    }
    else if (instr->second == S) {
        int rs1 = reg_map[args[2]];
        int rs2 = reg_map[args[0]];
        int imm = parseImmediate(args[1]);
        int funct3, opcode;
        if (mnemonic == "sb") funct3 = 0;
        else if (mnemonic == "sh") funct3 = 1;
        else if (mnemonic == "sw") funct3 = 2;
        opcode = 0x23;
        machineCode |= ((imm & 0xFE0) << 20) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | ((imm & 0x1F) << 7) | opcode;
    } else if (instr->second == B) {
        int rs1 = reg_map[args[0]];
        int rs2 = reg_map[args[1]];
        int imm = parseImmediate(args[2]);
        int funct3, opcode;

        if (mnemonic == "beq") funct3 = 0;
        else if (mnemonic == "bne") funct3 = 1;
        else if (mnemonic == "blt") funct3 = 4;
        else if (mnemonic == "bge") funct3 = 5;
        else if (mnemonic == "bltu") funct3 = 6;
        else if (mnemonic == "bgeu") funct3 = 7;

        opcode = 0x63;

        int imm_12 = (imm >> 12) & 0x1;
        int imm_10_5 = (imm >> 5) & 0x3F;
        int imm_4_1 = (imm >> 1) & 0xF;
        int imm_11 = (imm >> 11) & 0x1;

        machineCode |= (imm_12 << 31) | (imm_10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12)
                       | (imm_4_1 << 8) | (imm_11 << 7) | opcode;
    }
    else if (instr->second == U) {
        int rd = reg_map[args[0]];
        int imm = parseImmediate(args[1]);

        int opcode;
        if (mnemonic == "lui") opcode = 0x37;
        else if (mnemonic == "auipc") opcode = 0x17;
        machineCode |= (imm << 12) | (rd << 7) | opcode;
    } else if (instr->second == J) {
        int rd = reg_map[args[0]];
        int imm = parseImmediate(args[1]);
        int opcode = 0x6F;
        machineCode |= ((imm & 0x100000) << 11) | ((imm & 0x7FE) << 20) | ((imm & 0x800) << 9)
                       | ((imm & 0xFF000) << 12) | (rd << 7) | opcode;
    }

    return machineCode;
}

/*---------------------------------- работа кэша с ассемблером -------------------------------------------------------*/
int32_t readRegister(Simulation& simulation, int reg) {
    return simulation.getReg(reg);
}

void writeRegister(Simulation& simulation, int32_t reg, uint32_t value) {
    simulation.setReg(reg, value);
}

struct Command {
    int32_t reg1_{}, reg2_{}, reg3_{};
    int32_t offset_{};
    std::function<void(Simulation&)> cmd_{};

    Command(int32_t reg1_, int32_t reg2_, int32_t reg3_, int32_t offset_, const std::function<void(Simulation&)>& cmd_)
            : reg1_(reg1_), reg2_(reg2_), reg3_(reg3_), offset_(offset_), cmd_(cmd_) {};
};


/*--------------------------------------- работа с ассемблером -------------------------------------------------------*/
std::vector<Command> parseAssembly(const std::string &asmFile, std::vector<int8_t>& memory, std::vector<uint32_t>& binary) {
    std::vector<Command> commands;
    std::ifstream file(asmFile);
    std::string line;

    int address = 0;
    while (std::getline(file, line)) {
        line = line.substr(0, line.find('#'));
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
        std::replace(line.begin(), line.end(), ',', ' ');
        if (line.empty()) { continue; }

        std::istringstream iss(line);
        std::string mnemonic, arg1, arg2, arg3;
        iss >> mnemonic;

        if (mnemonic.back() == ':' || mnemonic[0] == '.') {
            continue;
        }

        auto instr = instrMap.find(mnemonic);
        if (instr == instrMap.end()) {
            std::cerr << "Unrecognized instruction mnemonic: " << mnemonic << std::endl;
            continue;
        }

        std::vector<std::string> args;
        if (instr->second == R || instr->second == I || instr->second == S || instr->second == B) {
            iss >> arg1 >> arg2 >> arg3;
            args.push_back(arg1);
            if (arg2.find('(') != std::string::npos && arg2.find(')') != std::string::npos) {
                std::size_t pos1 = arg2.find('(');
                std::size_t pos2 = arg2.find(')');
                args.push_back(arg2.substr(pos1 + 1, pos2 - pos1 - 1));
                args.push_back(arg2.substr(0, pos1));
            } else {
                args.push_back(arg2);
                args.push_back(arg3);
            }
        } else if (instr->second == U || instr->second == J) {
            iss >> arg1 >> arg2;
            args.push_back(arg1);
            args.push_back(arg2);
        }

        uint32_t machineCode = AssemblyToMachineCode(mnemonic, args, address);
        binary.push_back(machineCode);

        int64_t imm = -1;
        int rd = -1;
        int rs1 = -1;
        int rs2 = -1;

        /*---------------R's-------------*/
        if (instr->second == R) {
            rd = reg_map[args[0]];
            rs1 = reg_map[args[1]];
            rs2 = reg_map[args[2]];
            if (mnemonic == "add") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, rs1_val + rs2_val);
                });

            else if (mnemonic == "sub") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, rs1_val - rs2_val);
                });

            else if (mnemonic == "sll") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, rs1_val << rs2_val);
                });

            else if (mnemonic == "slt") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, (rs1_val < rs2_val) ? 1 : 0);
                });

            else if (mnemonic == "sltu") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, (static_cast<uint32_t>(rs1_val) < static_cast<uint32_t>(rs2_val)) ? 1 : 0);
                });

            else if (mnemonic == "xor") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, rs1_val ^ rs2_val);
                });

            else if (mnemonic == "srl") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, static_cast<uint32_t>(rs1_val) >> rs2_val);
                });

            else if (mnemonic == "sra") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, rs1_val >> rs2_val);
                });

            else if (mnemonic == "or") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, rs1_val | rs2_val);
                });

            else if (mnemonic == "and") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, rs1_val & rs2_val);
                });

            else if (mnemonic == "mul") commands.emplace_back(rd, rs1, rs2, 0, [rd, rs1, rs2](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    writeRegister(simulation, rd, rs1_val * rs2_val);
                });
        }
            /*---------------I's-------------*/
        else if (instr->second == I) {
            if (mnemonic == "jalr") {
                rd = reg_map[arg1];
                rs1 = reg_map[arg2];
                imm = parseImmediate(arg3);
            } else {
                rd = reg_map[args[0]];
                rs1 = reg_map[args[1]];
                imm = parseImmediate(args[2]);
            }

            if (mnemonic == "addi") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, rs1_val + imm);
                });

            else if (mnemonic == "slti") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, (rs1_val < imm) ? 1 : 0);
                });

            else if (mnemonic == "sltiu") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, (static_cast<uint32_t>(rs1_val) < static_cast<uint32_t>(imm)) ? 1 : 0);
                });

            else if (mnemonic == "xori") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, rs1_val ^ imm);
                });

            else if (mnemonic == "ori") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, rs1_val | imm);
                });

            else if (mnemonic == "andi") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, rs1_val & imm);
                });

            else if (mnemonic == "slli") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, rs1_val << imm);
                });

            else if (mnemonic == "srli") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, static_cast<uint32_t>(rs1_val) >> imm);
                });

            else if (mnemonic == "srai") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, rs1_val >> imm);
                });
            else if (mnemonic == "jalr") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    writeRegister(simulation, rd, pc + 4);
                    pc = rs1_val + imm;
                });

            if (mnemonic == "lb" || mnemonic == "lh" || mnemonic == "lw" || mnemonic == "lbu" || mnemonic == "lhu" || mnemonic == "ld") {
                rd = reg_map[args[0]];
                rs1 = reg_map[args[2]];
                imm = parseImmediate(args[1]);

                if (mnemonic == "lb") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm, &memory](Simulation& simulation) {
                        uint32_t rs1_val = readRegister(simulation, rs1);
                        uint32_t address = rs1_val + imm;
                        Address addr = decodeAddress(address);
                        simulation.request(addr, Type::r, memory);
                        uint32_t value = static_cast<int8_t>(memory[address]);
                        writeRegister(simulation, rd, value);
                    });

                else if (mnemonic == "lh") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm, &memory](Simulation& simulation) {
                        uint32_t rs1_val = readRegister(simulation, rs1);
                        uint32_t address = rs1_val + imm;
                        Address addr = decodeAddress(address);
                        simulation.request(addr, Type::r, memory);
                        uint32_t value = *reinterpret_cast<int16_t*>(&memory[address]);
                        writeRegister(simulation, rd, value);
                    });
                else if (mnemonic == "lw") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm, &memory](Simulation& simulation) {
                        uint32_t rs1_val = readRegister(simulation, rs1);
                        uint32_t address = rs1_val + imm;
                        Address addr = decodeAddress(address);
                        simulation.request(addr, Type::r, memory);
                        uint32_t value = *reinterpret_cast<int32_t*>(&memory[address]);
                        writeRegister(simulation, rd, value);
                    });
                else if (mnemonic == "lbu") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm, &memory](Simulation& simulation) {
                        uint32_t rs1_val = readRegister(simulation, rs1);
                        uint32_t address = rs1_val + imm;
                        Address addr = decodeAddress(address);
                        simulation.request(addr, Type::r, memory);
                        uint32_t value = static_cast<uint8_t>(memory[address]);
                        writeRegister(simulation, rd, value);
                    });
                else if (mnemonic == "lhu") commands.emplace_back(rd, rs1, 0, imm, [rd, rs1, imm, &memory](Simulation& simulation) {
                        uint32_t rs1_val = readRegister(simulation, rs1);
                        uint32_t address = rs1_val + imm;
                        Address addr = decodeAddress(address);
                        simulation.request(addr, Type::r, memory);
                        uint32_t value = *reinterpret_cast<uint16_t*>(&memory[address]);
                        writeRegister(simulation, rd, value);
                    });
            }
        }
            /*---------------B's-------------*/
        else if (instr->second == B) {
            rs1 = reg_map[arg1];
            rs2 = reg_map[arg2];
            imm = parseImmediate(arg3);

            if (mnemonic == "beq") commands.emplace_back(rs1, rs2, 0, imm, [rs1, rs2, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    if (rs1_val == rs2_val) pc += imm;
                });

            else if (mnemonic == "bne") commands.emplace_back(rs1, rs2, 0, imm, [rs1, rs2, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    if (rs1_val != rs2_val) pc += imm;
                });

            else if (mnemonic == "blt") commands.emplace_back(rs1, rs2, 0, imm, [rs1, rs2, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    if (rs1_val < rs2_val) pc += imm;
                });

            else if (mnemonic == "bge") commands.emplace_back(rs1, rs2, 0, imm, [rs1, rs2, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    if (rs1_val >= rs2_val) pc += imm;
                });

            else if (mnemonic == "bltu") commands.emplace_back(rs1, rs2, 0, imm, [rs1, rs2, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    if (static_cast<uint32_t>(rs1_val) < static_cast<uint32_t>(rs2_val)) pc += imm;
                });

            else if (mnemonic == "bgeu") commands.emplace_back(rs1, rs2, 0, imm, [rs1, rs2, imm](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    if (static_cast<uint32_t>(rs1_val) >= static_cast<uint32_t>(rs2_val)) pc += imm;
                });
        }
            /*---------------S's-------------*/
        else if (instr->second == S) {
            rs1 = reg_map[args[2]];
            rs2 = reg_map[args[0]];
            imm = parseImmediate(args[1]);

            if (mnemonic == "sb") commands.emplace_back(rs1, rs2, 0, imm, [rs1, rs2, imm, &memory](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    uint32_t address = rs1_val + imm;
                    Address addr = decodeAddress(address);
                    simulation.request(addr, Type::w, memory);
                    memory[address] = static_cast<int8_t>(rs2_val);
                });

            else if (mnemonic == "sh") commands.emplace_back(rs1, rs2, 0, imm, [rs1, rs2, imm, &memory](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    uint32_t address = rs1_val + imm;
                    Address addr = decodeAddress(address);
                    simulation.request(addr, Type::w, memory);
                    *reinterpret_cast<int16_t*>(&memory[address]) = static_cast<int16_t>(rs2_val);
                });

            else if (mnemonic == "sw") commands.emplace_back(rs1, rs2, 0, imm, [rs1, rs2, imm, &memory](Simulation& simulation) {
                    uint32_t rs1_val = readRegister(simulation, rs1);
                    uint32_t rs2_val = readRegister(simulation, rs2);
                    uint32_t address = rs1_val + imm;
                    Address addr = decodeAddress(address);
                    simulation.request(addr, Type::w, memory);
                    *reinterpret_cast<int32_t*>(&memory[address]) = static_cast<int32_t>(rs2_val);
                });
        }

            /*---------------U's-------------*/
        else if (instr->second == U) {
            rd = reg_map[arg1];
            imm = parseImmediate(arg2);

            if (mnemonic == "lui") commands.emplace_back(rd, 0, 0, imm, [rd, imm](Simulation& simulation) {
                    writeRegister(simulation, rd, imm << 12);
                });

            else if (mnemonic == "auipc") commands.emplace_back(rd, 0, 0, imm, [rd, imm](Simulation& simulation) {
                    writeRegister(simulation, rd, pc + (imm << 12));
                });
        }
            /*---------------J's-------------*/
        else if (instr->second == J) {
            rd = reg_map[arg1];
            imm = parseImmediate(arg2);

            if (mnemonic == "jal") commands.emplace_back(rd, 0, 0, imm, [rd, imm](Simulation& simulation) {
                    writeRegister(simulation, rd, pc + 4);
                    pc = imm;
                });
        }

        address += 4;
    }

    return commands;
}


/*--------------------------------------------------- main -----------------------------------------------------------*/
int main(int argc, char* argv[]) {
    std::string asmFile, binFile;
    ReplacementPolicy policy = ALL;

    try {
        if (argc == 1) throw std::runtime_error("No arguments were provided");

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--asm") {
                if (++i < argc) asmFile = argv[i];
                else throw std::runtime_error("No assembly file specified.");
            } else if (arg == "--bin") {
                if (++i < argc) binFile = argv[i];
                else throw std::runtime_error("No binary file specified.");
            } else if (arg == "--replacement") {
                if (++i < argc) policy = static_cast<ReplacementPolicy>(std::stoi(argv[i]));
                else throw std::runtime_error("No replacement policy specified.");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing command-line arguments: " << e.what() << std::endl;
        return 1;
    }

    try {

        CacheLRU cache_LRU; CachePLRU cache_pLRU;
        CacheSimulator cache_LRU_sim(&cache_LRU); CacheSimulator cache_pLRU_sim(&cache_pLRU);
        Simulation simulation({cache_LRU_sim, cache_pLRU_sim}, policy);
        std::vector<uint32_t> binary;
        auto commands = parseAssembly(asmFile, memory, binary);

        /*------------------- запись бин кода в файл ---------------*/
        std::ofstream binFileOut(binFile, std::ios::binary);
        for (auto code : binary) {
            binFileOut.write(reinterpret_cast<const char*>(&code), sizeof(code));
        }
        binFileOut.close();


        /*---------------------- работа с кэшем --------------------*/
        while (pc / 4 < (int)commands.size()) {
            auto save_pc = pc;
            commands[pc / 4].cmd_(simulation);
            if (pc == save_pc) pc += 4;
            if (pc == 0) break;
        }

        simulation.printResult();


    } catch (const std::exception& e) {
        std::cerr << "Error during simulation: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
