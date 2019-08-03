#include "hksdisasm.h"
#pragma comment(lib, "Ws2_32.lib")



FILE* hks_file;
int pc;
int function_ptr = 0;
int const_ptr;
int inst_ptr;
char* path = NULL;
int* const_list;
int* line_numbers;
LOCAL* locals;
int* upvalues;

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: .\\hksdisasm <filename>\n");
		return 0;
	}

	hks_file = fopen(argv[1], "rb");

	if (!hks_file)
	{
		printf("Cannot open file \"%s\": %s", argv[1], strerror(errno));
		return 1;
	}

	char lua_header[12];
	fread(lua_header, 1, 12, hks_file);

	if (strcmp(lua_header, "\x1b\x4c\x75\x61\x51\x0e\x00\x04\x08\x04\x04")) {
		printf("lol\n");
		return 1;
	}
	fseek(hks_file, 0xee, SEEK_SET);

	parse_function();
	//fseek(hks_file, function_ptr, SEEK_SET);
	//parse_main();

	fclose(hks_file);

}

int parse_function() {

	printf("\n");

	int num_params;
	int num_slots;
	int header_unk1;
	int header_unk2;
	int num_instructions;
	int num_const_slots;

	upvalues = NULL;
	locals = NULL;

	//printf("function at %d\n", function_ptr);

	fseek(hks_file, 4, SEEK_CUR);
	freadbe_int(&num_params, 1, hks_file);
	fread(&header_unk1, 1, 1, hks_file);
	freadbe_int(&num_slots, 1, hks_file);
	freadbe_int(&header_unk2, 1, hks_file);
	freadbe_int(&num_instructions, 1, hks_file);

	int* instructions = malloc(num_instructions * 4);

	if (ftell(hks_file) % 4 > 0)
		fseek(hks_file, 4 - ftell(hks_file) % 4, SEEK_CUR);

	inst_ptr = ftell(hks_file);

	freadbe_int(instructions, num_instructions, hks_file);
	freadbe_int(&num_const_slots, 1, hks_file);
	const_ptr = ftell(hks_file);

	build_constant_list(num_const_slots);

	int num_locals;
	int num_upvalues;
	int ln_function_begin;
	int ln_function_end;

	fseek(hks_file, 8, SEEK_CUR);
	freadbe_int(&num_locals, 1, hks_file);
	freadbe_int(&num_upvalues, 1, hks_file);
	freadbe_int(&ln_function_begin, 1, hks_file);
	freadbe_int(&ln_function_end, 1, hks_file);
	fseek(hks_file, 4, SEEK_CUR);

	int path_length;

	//printf("path at %d\n", ftell(hks_file));

	freadbe_int(&path_length, 1, hks_file);

	if (path_length) {
		if (path) {
			free(path);
			path = NULL;
		}

		path = malloc(path_length);

		fread(path, 1, path_length, hks_file);
	}

	fseek(hks_file, 4, SEEK_CUR);

	int name_length;
	char* function_name;

	//printf("name at %d\n", ftell(hks_file));

	freadbe_int(&name_length, 1, hks_file);

	function_name = malloc(name_length);

	fread(function_name, 1, name_length, hks_file);

	if (name_length && !strcmp(function_name, "(main chunk)")) {
		printf("main <%s:%d,%d> (%d instructions, %d bytes at %08X)\n", path, ln_function_begin, ln_function_end, num_instructions, num_instructions * 4, inst_ptr);
	}
	else {
		printf("function <%s:%d,%d> (%d instructions, %d bytes at %08X)\n", path, ln_function_begin, ln_function_end, num_instructions, num_instructions * 4, inst_ptr);
	}

	//printf("%s, %s\n", path, function_name);

	line_numbers = malloc(4 * num_instructions);

	freadbe_int(line_numbers, num_instructions, hks_file);

	if (num_locals) {

		locals = malloc(sizeof(LOCAL) * num_locals);

		for (int i = 0; i < num_locals; i++) {

			fseek(hks_file, 4, SEEK_CUR);

			LOCAL local;

			local.str_offset = ftell(hks_file);

			int str_length = 0;

			freadbe_int(&str_length, 1, hks_file);
			fseek(hks_file, str_length, SEEK_CUR);

			freadbe_int(&local.start, 1, hks_file);
			freadbe_int(&local.end, 1, hks_file);

			locals[i] = local;

		}
	}

	if (num_upvalues) {
		//if (!num_locals) fseek(hks_file, 4, SEEK_CUR);

		upvalues = malloc(4 * num_upvalues);

		for (int i = 0; i < num_upvalues; i++) {

			fseek(hks_file, 4, SEEK_CUR);

			upvalues[i] = ftell(hks_file);

			int str_length = 0;

			freadbe_int(&str_length, 1, hks_file);
			fseek(hks_file, str_length, SEEK_CUR);
		}

		//printf("end of upvalues at %d\n", ftell(hks_file));
	}

	int num_functions;

	freadbe_int(&num_functions, 1, hks_file);

	function_ptr = ftell(hks_file);

	printf("%d params, %d slots, %d upvalues, %d locals, %d constants, %d functions\n", num_params, num_slots, num_upvalues, num_locals, num_const_slots, num_functions);

	parse_instructions(instructions, num_instructions);

	print_constants(num_const_slots);

	print_locals(num_locals);

	print_upvalues(num_upvalues);

	if (instructions)
		free(instructions);

	if (locals)
		free(locals);

	if (upvalues)
		free(upvalues);

	if (const_list) {
		free(const_list);
		const_list = NULL;
	}

	if (line_numbers) {
		free(line_numbers);
		line_numbers = NULL;
	}

	for (int i = 0; i < num_functions; i++) {
		fseek(hks_file, function_ptr, SEEK_SET);
		parse_function();
	}

	return 0;
}

size_t freadbe_int(int* _Buffer, size_t _ElementCount, FILE* _Stream) {
	size_t result = fread(_Buffer, 4, _ElementCount, _Stream);
	
	for (unsigned int i = 0; i < result; i++) {
		_Buffer[i] = htonl(_Buffer[i]);
	}
	return result;
}

int parse_instructions(int* instructions, int num_instructions) {
	int curr_inst;
	int opcode = 0;
	int args;
	char* constant = NULL;
	for (int i = 0; i < num_instructions; i++) {
		curr_inst = instructions[i];
		opcode = (curr_inst & 0xff000000) >> 25;
		//printf("inst: %d, opcode: %d\n", curr_inst, opcode);
		int args = curr_inst >> 7;
		int A;
		int B;
		int C;
		int Ax;
		int Bx;
		int sBx;

		switch (opcode) {
		case HKS_OPCODE_TEST:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_CALL_I:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_EQ:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_MOVE:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_SELF:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_RETURN:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_GETTABLE_S:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_LOADBOOL:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_SETFIELD:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 1, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_SETTABLE_S:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_SETTABLE_S_BK:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_LOADK:
			getABx(curr_inst, &A, &Bx);
			constant = getK(Bx, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, Bx, constant);
			break;
		case HKS_OPCODE_LOADNIL:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_SETGLOBAL:
			getABx(curr_inst, &A, &Bx);
			constant = getK(Bx, 1, 0);
			printf("\t %d \t [%d] \t %-20s %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, Bx, constant);
			break;
		case HKS_OPCODE_JMP:
			sBx = (curr_inst & 0x1ffff00) >> 8;

			if (sBx & 0x10000) {
				printf("\t %d \t [%d] \t %-20s %d \t; to %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], (unsigned short)sBx, i + (unsigned short)sBx + 3);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d \t; to %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], (short)sBx, i + 2);
			}

			break;
		case HKS_OPCODE_GETUPVAL:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_ADD:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_ADD_BK:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_SUB:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_SUB_BK:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_MUL:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_MUL_BK:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_DIV:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_DIV_BK:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_MOD:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_MOD_BK:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_POW:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_POW_BK:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_NEWTABLE:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_UNM:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_LEN:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_LT:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_LT_BK:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_LE:
			getABC(curr_inst, &A, &B, &C);

			if (C < 0) {
				constant = getK(C, 0, 0);
				printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			}

			break;
		case HKS_OPCODE_LE_BK:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(B, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_CONCAT:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_FORPREP:
			A = curr_inst & 0xff;
			sBx = (curr_inst & 0x1ffff00) >> 8;

			if (sBx & 0x10000) {
				printf("\t %d \t [%d] \t %-20s %d %d \t; to %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, (unsigned short)sBx, i + (unsigned short)sBx + 3);
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d \t; to %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, (short)sBx, i + 2);
			}

			break;
		case HKS_OPCODE_FORLOOP:
			A = curr_inst & 0xff;
			sBx = (curr_inst & 0x1ffff00) >> 8;

			if (sBx & 0x10000) {
				printf("\t %d \t [%d] \t %-20s %d %d \t; to %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, (unsigned short)sBx, i + (unsigned short)sBx + 3); //???? completely untested
			}
			else {
				printf("\t %d \t [%d] \t %-20s %d %d \t; to %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, (short)sBx, i + (short)sBx + 3);
			}

			break;
		case HKS_OPCODE_SETLIST:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_CLOSURE:
			getABx(curr_inst, &A, &Bx);
			printf("\t %d \t [%d] \t %-20s %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, Bx);
			break;
		case HKS_OPCODE_VARARG:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_CALL_I_R1:
			getABC(curr_inst, &A, &B, &C);
			printf("\t %d \t [%d] \t %-20s %d %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C);
			break;
		case HKS_OPCODE_GETFIELD_R1:
			getABC(curr_inst, &A, &B, &C);
			constant = getK(C, 0, 0);
			printf("\t %d \t [%d] \t %-20s %d %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, B, C, constant);
			break;
		case HKS_OPCODE_DATA:
			getABx(curr_inst, &A, &Bx);
			printf("\t %d \t [%d] \t %-20s %d %d\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, Bx);
			break;
		case HKS_OPCODE_GETGLOBAL_MEM:
			getABx(curr_inst, &A, &Bx);
			constant = getK(Bx, 1, 0);
			printf("\t %d \t [%d] \t %-20s %d %d \t; %s\n", i + 1, line_numbers[i], hks_opcodes[opcode], A, Bx, constant);
			break;
		default:
			printf("opcode %d (%s) not implemented yet\n", opcode, hks_opcodes[opcode]);
			break;
		}

		if (constant) {
			free(constant);
			constant = NULL;
		}
	}

	return 0;
}

int print_constants(int num_const_slots) {
	printf("constants (%d) for %08X\n", num_const_slots, inst_ptr);
	for (int i = 0; i < num_const_slots; i++) {
		if (const_list[i] != 1) {
			printf("\t%d\t%s\n", i, getK(i, 0, 1));
		}
	}

	return 0;
}

int print_locals(int num_locals) {
	printf("locals (%d) for %08X\n", num_locals, inst_ptr);
	for (int i = 0; i < num_locals; i++) {
		fseek(hks_file, locals[i].str_offset, SEEK_SET);
		int length = 0;

		freadbe_int(&length, 1, hks_file);

		char* str = malloc(length);

		fread(str, 1, length, hks_file);

		printf("\t%d\t%s\t%d\t%d\n", i, str, locals[i].start, locals[i].end);
	}

	return 0;
}

int print_upvalues(int num_upvalues) {
	printf("upvalues (%d) for %08X\n", num_upvalues, inst_ptr);
	for (int i = 0; i < num_upvalues; i++) {
		fseek(hks_file, upvalues[i], SEEK_SET);
		int length = 0;

		freadbe_int(&length, 1, hks_file);

		char* str = malloc(length);

		fread(str, 1, length, hks_file);

		printf("\t%d\t%s\n", i, str);
	}

	return 0;
}

void getABC(int curr_inst, int* A, int* B, int* C) {
	*A = curr_inst & 0xff;
	*C = (curr_inst & 0x1ff00) >> 8;
	*B = (curr_inst & 0x1fe0000) >> 17;

	if (*B & 0x100) *B = -(*B & 0xff);
	if (*C & 0x100) *C = -(*C & 0xff);
}

void getABx(int curr_inst, int* A, int* Bx) {
	*A = curr_inst & 0xff;
	*Bx = (curr_inst & 0x1ffff00) >> 8;
}

char* getK(int index, int is_global, int is_for_list) {
	if (index < 0) index = -index;
	fseek(hks_file, const_list[index], SEEK_SET);

	char* buffer;
	char type = 0;

	fread(&type, 1, 1, hks_file);

	switch (type) {
	case 1:;
		char bool_val = 0;
		fread(&bool_val, 1, 1, hks_file);

		if (bool_val) {
			buffer = malloc(5); //doing this so we can "free" later...
			sprintf(buffer, "true");
		}
		else {
			buffer = malloc(6);
			sprintf(buffer, "false");
		}
		break;
	case 3:;
		int number;
		freadbe_int(&number, 1, hks_file);
		buffer = malloc(32);
		sprintf(buffer, "%f", *(float*)&number);

		break;
	case 4:
		fseek(hks_file, 7, SEEK_CUR);

		char length = 0;

		fread(&length, 1, 1, hks_file);

		buffer = malloc(length);

		fread(buffer, 1, length, hks_file);

		if (is_for_list || !is_global) {
			char* new_buffer = malloc(length + 2);

			sprintf(new_buffer, "\"%s\"", buffer);

			free(buffer);

			buffer = new_buffer;
		}
		break;
	default:
		buffer = "";
	}

	return buffer;
}

int build_constant_list(int num_const_slots) {
	if (!num_const_slots) return 1;

	int curr_slot = 0;

	const_list = malloc(4 * num_const_slots);

	char type;

	while (curr_slot < num_const_slots) {
		fread(&type, 1, 1, hks_file);

		char test_byte = 0;

		switch (type) {
		case 1:
			const_list[curr_slot] = ftell(hks_file) - 1;
			curr_slot++;

			fseek(hks_file, 1, SEEK_CUR);
			break;
		case 3:
			const_list[curr_slot] = ftell(hks_file) - 1;
			curr_slot++;

			fseek(hks_file, 4, SEEK_CUR);

			while (test_byte == 0 && curr_slot < num_const_slots) {
				fread(&test_byte, 1, 1, hks_file);

				if (!test_byte) {
					const_list[curr_slot] = 1;
					curr_slot++;
				}
				else {
					fseek(hks_file, ftell(hks_file) - 1, SEEK_SET);
				}
			}
			break;
		case 4:
			const_list[curr_slot] = ftell(hks_file) - 1;
			curr_slot++;

			int str_size = 0;

			fseek(hks_file, 7, SEEK_CUR);
			fread(&str_size, 1, 1, hks_file);
			fseek(hks_file, str_size, SEEK_CUR);

			while (test_byte == 0 && curr_slot < num_const_slots) {
				fread(&test_byte, 1, 1, hks_file);

				if (!test_byte) {
					const_list[curr_slot] = 1;
					curr_slot++;
				}
				else {
					fseek(hks_file, ftell(hks_file) - 1, SEEK_SET);
				}
			}
			break;
		}
	}

	return 0;
}

