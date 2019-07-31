#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <winsock2.h>

#define HKS_OPCODE_GETFIELD 0
#define HKS_OPCODE_TEST 1
#define HKS_OPCODE_CALL_I 2
#define HKS_OPCODE_CALL_C 3
#define HKS_OPCODE_EQ 4
#define HKS_OPCODE_EQ_BK 5
#define HKS_OPCODE_GETGLOBAL 6
#define HKS_OPCODE_MOVE 7
#define HKS_OPCODE_SELF 8
#define HKS_OPCODE_RETURN 9
#define HKS_OPCODE_GETTABLE_S 10
#define HKS_OPCODE_GETTABLE_N 11
#define HKS_OPCODE_GETTABLE 12
#define HKS_OPCODE_LOADBOOL 13
#define HKS_OPCODE_TFORLOOP 14
#define HKS_OPCODE_SETFIELD 15
#define HKS_OPCODE_SETTABLE_S 16
#define HKS_OPCODE_SETTABLE_S_BK 17
#define HKS_OPCODE_SETTABLE_N 18
#define HKS_OPCODE_SETTABLE_N_BK 19
#define HKS_OPCODE_SETTABLE 20
#define HKS_OPCODE_SETTABLE_BK 21
#define HKS_OPCODE_TAILCALL_I 22
#define HKS_OPCODE_TAILCALL_C 23
#define HKS_OPCODE_TAILCALL_M 24
#define HKS_OPCODE_LOADK 25
#define HKS_OPCODE_LOADNIL 26
#define HKS_OPCODE_SETGLOBAL 27
#define HKS_OPCODE_JMP 28
#define HKS_OPCODE_CALL_M 29
#define HKS_OPCODE_CALL 30
#define HKS_OPCODE_INTRINSIC_INDEX 31
#define HKS_OPCODE_INTRINSIC_NEWINDEX 32
#define HKS_OPCODE_INTRINSIC_SELF 33
#define HKS_OPCODE_INTRINSIC_INDEX_LITERAL 34
#define HKS_OPCODE_INTRINSIC_NEWINDEX_LITERAL 35
#define HKS_OPCODE_INTRINSIC_SELF_LITERAL 36
#define HKS_OPCODE_TAILCALL 37
#define HKS_OPCODE_GETUPVAL 38
#define HKS_OPCODE_SETUPVAL 39
#define HKS_OPCODE_ADD 40
#define HKS_OPCODE_ADD_BK 41
#define HKS_OPCODE_SUB 42
#define HKS_OPCODE_SUB_BK 43
#define HKS_OPCODE_MUL 44
#define HKS_OPCODE_MUL_BK 45
#define HKS_OPCODE_DIV 46
#define HKS_OPCODE_DIV_BK 47
#define HKS_OPCODE_MOD 48
#define HKS_OPCODE_MOD_BK 49
#define HKS_OPCODE_POW 50
#define HKS_OPCODE_POW_BK 51
#define HKS_OPCODE_NEWTABLE 52
#define HKS_OPCODE_UNM 53
#define HKS_OPCODE_NOT 54
#define HKS_OPCODE_LEN 55
#define HKS_OPCODE_LT 56
#define HKS_OPCODE_LT_BK 57
#define HKS_OPCODE_LE 58
#define HKS_OPCODE_LE_BK 59
#define HKS_OPCODE_CONCAT 60
#define HKS_OPCODE_TESTSET 61
#define HKS_OPCODE_FORPREP 62
#define HKS_OPCODE_FORLOOP 63
#define HKS_OPCODE_SETLIST 64
#define HKS_OPCODE_CLOSE 65
#define HKS_OPCODE_CLOSURE 66
#define HKS_OPCODE_VARARG 67
#define HKS_OPCODE_TAILCALL_I_R1 68
#define HKS_OPCODE_CALL_I_R1 69
#define HKS_OPCODE_SETUPVAL_R1 70
#define HKS_OPCODE_TEST_R1 71
#define HKS_OPCODE_NOT_R1 72
#define HKS_OPCODE_GETFIELD_R1 73
#define HKS_OPCODE_SETFIELD_R1 74
#define HKS_OPCODE_NEWSTRUCT 75
#define HKS_OPCODE_DATA 76
#define HKS_OPCODE_SETSLOTN 77
#define HKS_OPCODE_SETSLOTI 78
#define HKS_OPCODE_SETSLOT 79
#define HKS_OPCODE_SETSLOTS 80
#define HKS_OPCODE_SETSLOTMT 81
#define HKS_OPCODE_CHECKTYPE 82
#define HKS_OPCODE_CHECKTYPES 83
#define HKS_OPCODE_GETSLOT 84
#define HKS_OPCODE_GETSLOTMT 85
#define HKS_OPCODE_SELFSLOT 86
#define HKS_OPCODE_SELFSLOTMT 87
#define HKS_OPCODE_GETFIELD_MM 88
#define HKS_OPCODE_CHECKTYPE_D 89
#define HKS_OPCODE_GETSLOT_D 90
#define HKS_OPCODE_GETGLOBAL_MEM 91


size_t freadbe_int(int*, size_t, FILE*);
int parse_instructions(int*, int);
int parse_function();
int parse_function();
int print_constants(int);
int print_locals(int);
int print_upvalues(int);
int build_constant_list(int);
void getABC(int, int*, int*, int*);
void getABx(int, int*, int*);
char* getK(int, int, int);

char* hks_opcodes[] = { "GETFIELD", 
						"TEST",
						"CALL_I",
						"CALL_C",
						"EQ",
						"EQ_BK",
						"GETGLOBAL",
						"MOVE",
						"SELF",
						"RETURN",
						"GETTABLE_S",
						"GETTABLE_N",
						"GETTABLE",
						"LOADBOOL",
						"TFORLOOP",
						"SETFIELD",
						"SETTABLE_S",
						"SETTABLE_S_BK",
						"SETTABLE_N",
						"SETTABLE_N_BK",
						"SETTABLE",
						"SETTABLE_BK",
						"TAILCALL_I",
						"TAILCALL_C",
						"TAILCALL_M",
						"LOADK",
						"LOADNIL",
						"SETGLOBAL",
						"JMP",
						"CALL_M",
						"CALL",
						"INTRINSIC_INDEX",
						"INTRINSIC_NEWINDEX",
						"INTRINSIC_SELF",
						"INTRINSIC_INDEX_LITERAL",
						"INTRINSIC_NEWINDEX_LITERAL",
						"INTRINSIC_SELF_LITERAL",
						"TAILCALL",
						"GETUPVAL",
						"SETUPVAL",
						"ADD",
						"ADD_BK",
						"SUB",
						"SUB_BK",
						"MUL",
						"MUL_BK",
						"DIV",
						"DIV_BK",
						"MOD",
						"MOD_BK",
						"POW",
						"POW_BK",
						"NEWTABLE",
						"UNM",
						"NOT",
						"LEN",
						"LT",
						"LT_BK",
						"LE",
						"LE_BK",
						"CONCAT",
						"TESTSET",
						"FORPREP",
						"FORLOOP",
						"SETLIST",
						"CLOSE",
						"CLOSURE",
						"VARARG",
						"TAILCALL_I_R1",
						"CALL_I_R1",
						"SETUPVAL_R1",
						"TEST_R1",
						"NOT_R1",
						"GETFIELD_R1",
						"SETFIELD_R1",
						"NEWSTRUCT",
						"DATA",
						"SETSLOTN",
						"SETSLOTI",
						"SETSLOT",
						"SETSLOTS",
						"SETSLOTMT",
						"CHECKTYPE",
						"CHECKTYPES",
						"GETSLOT",
						"GETSLOTMT",
						"SELFSLOT",
						"SELFSLOTMT",
						"GETFIELD_MM",
						"CHECKTYPE_D",
						"GETSLOT_D",
						"GETGLOBAL_MEM"
					};

typedef struct {
	int str_offset;
	int start;
	int end;
} LOCAL;