#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

void readin(const char* filename, char* output){
	FILE *fp = fopen(filename, "rb");
	if(!fp) printf("bad file.\n"), exit(1);
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fread(output, size, 1, fp);
	fclose(fp);
	output[size] = 0;
	
}

int opcount(const char* c){
	int count = 0;
	char op = *c;
	while(*c++ == op) ++count;
	return count;
}

void updatetargetout(char **targetout){
	*targetout += strlen(*targetout);
}

int main(int argc, const char **argv){
	if(argc != 3)
		printf("Invalid options. Expected:\n\tbfc input output"),
		exit(1);

	char src[1024];
	char *c = src;
	readin(argv[1], src);

	int bracketlvl = -1;
	int bracketlvltimes[1024] = {0};

	char *mainout = calloc(1, 1024*16);
	char *funcbuffout = calloc(1, 1024*16);
	char *targetout = mainout;

	char *mainoutinit = mainout;
	char *funcbuffoutinit = funcbuffout;

	sprintf(targetout,
		"\textern printf\n"
		"\textern putchar\n"
		"\textern getchar\n"
		"\tglobal main\n\n"
		"\tsection .text\n"
		"main:\n"
		"\tpush rbx\n"
		"\tlea rbx, [rel pptr]\n"
	);
	updatetargetout(&targetout);

	while(*c != 0){
		int opc = opcount(c);
		switch(*c){
			case '>': sprintf(targetout, "\tadd rbx, %i\n", opc); updatetargetout(&targetout); c += opc-1; break;
			case '<': sprintf(targetout, "\tsub rbx, %i\n", opc); updatetargetout(&targetout); c += opc-1; break;
			case '+': sprintf(targetout, "\tadd BYTE [rbx], %i\n", opc); updatetargetout(&targetout); c += opc-1; break;
			case '-': sprintf(targetout, "\tsub BYTE [rbx], %i\n", opc); updatetargetout(&targetout); c += opc-1; break;
			case '.': sprintf(targetout,
					"\tmov cl, BYTE [rbx]\n"
					"\tcall putchar\n");
					updatetargetout(&targetout); 
			break;
			
			case '/': sprintf(targetout,
					"\tlea rcx, [rel fmt]\n"
					"\txor rdx, rdx\n"
					"\tmov dl, BYTE [rbx]\n"
					"\tcall printf\n");
					updatetargetout(&targetout); 
			break;

			case ',': sprintf(targetout,
					"\tcall getchar\n"
					"\tmov BYTE [rbx], al\n");
					updatetargetout(&targetout); 
			break;
			
			case '[':
				bracketlvl++;
				bracketlvltimes[bracketlvl]++;
				sprintf(targetout,
				".loopOpen%i_%i:\n"
				"\tcmp BYTE [rbx], 0\n"
				"\tjz .loopClose%i_%i\n",
				bracketlvl, bracketlvltimes[bracketlvl], bracketlvl, bracketlvltimes[bracketlvl]);
				updatetargetout(&targetout); 
			break;

			case ']':
				sprintf(targetout,
				".loopClose%i_%i:\n"
				"\tcmp BYTE [rbx], 0\n"
				"\tjnz .loopOpen%i_%i\n",
				bracketlvl, bracketlvltimes[bracketlvl], bracketlvl, bracketlvltimes[bracketlvl]);
				bracketlvl--;
				updatetargetout(&targetout); 
			break;

			case '{':{
				const char *funcend = c;
				while(isspace(*(funcend-1))) funcend--;
				const char *funcname = funcend;
				while(isalnum(*(--funcname-1)));
				mainout = targetout;
				targetout = funcbuffout;
				sprintf(targetout, "\nfn_%.*s:\n", funcend-funcname, funcname);
				updatetargetout(&targetout); 
			}break;

			case '}':
				sprintf(targetout, "\tret\n\n");
				updatetargetout(&targetout); 
				funcbuffout = targetout;
				targetout = mainout;
			break;

			case '$':{
				const char *funcname = c+1;
				while(isalnum(*++c));
				sprintf(targetout, "\tcall fn_%.*s\n", c-funcname, funcname);
				updatetargetout(&targetout); 
				--c; //questionable
			}break;
		}
		++c;
	}

	FILE *out = fopen(argv[2], "w");
	fprintf(out, "%s\tpop rbx\n\tret\n%s\n\tsection .data\nfmt: db \"%%i \", 0\npptr: times 30000 db 0", mainoutinit, funcbuffoutinit);
	fclose(out);

	free(mainoutinit);
	free(funcbuffoutinit);
}

//+[/[>+<-]++++++++++.>+] print integers

/*
#arg
z{ [-] }

from #to
addl{ <[>+<-]> }
subl{ <[>-<-]> }
movl{ $z $addl }

#to from
addr{ >[<+>-]< }
subr{ >[<->-]< }
movr{ $z $addr }

from #to 0
cpyl{
    > $z <
    $movl > $movl
    [
        <<+
        >+
        >-
    ]
    <
}

#to from 0
addrND{
    >> $cpyl
    << $addr
    > $movr
    <
}

##0 ... n3 n2 #n1
sum{ < [ $addr < ] $movr }


helloworld{++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.}
$helloworld
*/