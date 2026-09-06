#include <stdio.h>
#include <stdlib.h>
#include "mvm.h"

static void repl(Vm*vm)
{
    char line[1024];
    for(;;)
    {
        printf("> ");
        if(!fgets(line,sizeof(line),stdin))
        {
            printf("\n");
            break;
        }
        mvm_interpret_result(line,vm);
    }
}

static char* read_file(const char *path)
{
    FILE *fp = fopen(path,"rb");
    if(fp==NULL)
    {
        fprintf(stderr,"Could not open file \"%s\".\n",path);
        exit(74);
    }
    fseek(fp, 0L,SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    char *buffer = (char*)malloc(size+1);
    if(buffer==NULL)
    {
        fprintf(stderr,"Not enough memory to read \"%s\".\n",path);
        exit(74);
    }
    size_t bytes_read = fread(buffer,sizeof(char),size,fp);
    if(bytes_read < size)
    {
        fprintf(stderr,"Could not read file \"%s\".\n",path);
        exit(74);
    }
    buffer[bytes_read] = '\0';

    fclose(fp);
    return buffer;
}

static void run_file(const char *path,Vm*vm)
{
    char *source = read_file(path);
    Result result = mvm_interpret_result(source,vm);
    free(source);

    if(result==RESULT_COMPILE_ERROR) exit(65);
    if(result==RESULT_RUNTIME_ERROR) exit(70);
}


int main(int argc, char *argv[])
{
    Vm vm;
    mvm_init(&vm);

    if(argc==1)
    {
        repl(&vm);
    }else if (argc==2) 
    {
        run_file(argv[1], &vm);
    }else 
    {
        fprintf(stderr, "Usage cmurin [path]\n");
        exit(64);
    }
    mvm_free(&vm);
    return 0;
}