#include <stdio.h>
#include <stdlib.h>

//类型的本质是对存储区间的划分
typedef struct keyval{int a;int b;} keyval;

keyval foo(const char* sa,const char* sb){
	keyval kv = {atoi(sa),atoi(sb)};
	//atoi() 函数还是比较安全的,如果是不规范的字符串,转换的结果默认为0
	return kv;
}

int main(int argc,char** argv){
	if(argc != 3){
		printf("usage: foo <a> <b>\n");
		return -1;
	}
	
	keyval kv = foo(argv[1],argv[2]);

	printf("%d + %d = %d\n",kv.a,kv.b,kv.a+kv.b);

	return 0;
}
