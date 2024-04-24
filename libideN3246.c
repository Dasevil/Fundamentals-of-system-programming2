#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include "plugin_api.h"

#define FLOAT_NUMBER "float"			//определяем имя long opt
static char *g_plugin_purpose = "Поиск текстовой записи чисел типа float, значение которых находится в указанном промежутке";
static char *g_plugin_author = "Ильменская Дарья Евгеньевна N3246";
static char *g_lib_name = "libideN3246.c";


static struct plugin_option g_po_arr[] = {		//опция типа long opt
/*
    struct plugin_option {
        struct option {
           const char *name;		
           int         has_arg;
           int        *flag;
           int         val;
        } opt,
        char *opt_descr
    	}
*/
        {
            {
                        FLOAT_NUMBER,			//имя длинной опции	
                        required_argument,		//требуется ли аргумен к текущей опции	
                        0, 0,				
            },
                "Поиск текстовой записи чисел типа float, значение которых находится в указанном промежутке"			//описание длинной опции
        }
};

struct input_args{	//структура, которую возвращаем по окончании проверки введенного флага(optarg)
	float b;	//вещественное число для проверки
	float r;	//определяет область подходящих значений
	int result;	//прошла ли проверку строка
};
 
struct input_args * input_check(char *number,struct input_args * args);	  //функция возвращающая предыдущую функцию; определяет введена ли правильно строка
  
static int g_po_arr_len = sizeof(g_po_arr)/sizeof(g_po_arr[0]);
int plugin_get_info(struct plugin_info* ppi) {				

    if (!ppi) {
        fprintf(stderr, "ERROR: invalid argument\n");
        return -1;
    }

    ppi->plugin_purpose = g_plugin_purpose;		
    ppi->plugin_author = g_plugin_author;		//заполнение информации по плагину
    ppi->sup_opts_len = g_po_arr_len;
    ppi->sup_opts = g_po_arr;
    return 0;
}

int plugin_process_file(const char *fname, struct option in_opts[], size_t in_opts_len) {

    int ret = -1; 				    // Возвращение ошибки по дефолту
    struct input_args *argsrange=NULL;	            //структура возвращающая информацию при проверке flag
    char *DEBUG = getenv("LAB1DEBUG");		    //определение debug - переменной окружения	
    					

    if (!fname || !in_opts || !in_opts_len) {	//обязательно передаются имя, опция, длина опции
        errno = EINVAL;				 
        return -1;
    }
    
    if (DEBUG) {				//debug информация по введенной опции и аргументе
        for (size_t i = 0; i < in_opts_len; i++) {
            fprintf(stderr, "DEBUG: %s: Got option '%s' with arg '%s'\n",
                g_lib_name, in_opts[i].name, (char*)in_opts[i].flag);
        }
    }

    char* input_string=0;					    							
    for (size_t i = 0; i < in_opts_len; ++i) {
        if (!strcmp(in_opts[i].name, FLOAT_NUMBER)) {		
            input_string = (char*)in_opts[i].flag;		//получаем flag в виде строки и записываем в переменную
        }
        
    }
    
    char* values=malloc(sizeof(char)*((int)strlen(input_string)+1));	//выделение памяти под переменную, в которой будем хранить значение flag
    for(int i=0; i<(int)strlen(input_string)+1;i++){
   	 values[i]=input_string[i];
    }
    values[(int)strlen(input_string)]='\0';				//конец строки
    
        
    FILE* fp = fopen(fname, "r"); 					//функция fopen открывает файл, после чего связывает его с потоком 
    if (fp == NULL) {   
	fprintf(stderr,"Ошибка чтения файла '%s'\n", fname);  	 
	return -1;
    } 

    int fd = open(fname, O_RDONLY);		//функция open преобразует путь к файлу в описатель файла для read и write 	
    if (fd < 0) {				
    	if (DEBUG) {
            fprintf(stderr, "DEBUG: ошибка открытия файла %s\n", fname);
        }			
        return -1;
    }
    
    int saved_errno = 0;    			
    struct stat st = {0};			//структура содержащая полную информацию по текущему файлу
    int res = fstat(fd, &st);
    if (res < 0) {				//при ошибке fstat возвращает -1
    	if (DEBUG) {
            fprintf(stderr, "DEBUG: ошибка возврата информации о файле %s\n", fname);
        }			
        saved_errno = errno;			
        goto END;
    }
      

    if((argsrange=input_check(values, argsrange))==NULL){	//проверка аргумента и возвращение структуры, содержащей информацию по проверке
    	printf("Аргумент к опции был введен неверно\n");
    	goto END;
    }
    	
    if (st.st_size == 0) {				
        if (DEBUG) {
            fprintf(stderr, "DEBUG: Открытый файл %s пустой\n", fname);
        }									//каждый файл должен содержать хоть что-то
        saved_errno = ERANGE;
        goto END;		
    }

    char *stringnum=NULL;				//получение числа из строки по разделителю ' ' 
    char *fileline=NULL;   				//строка, содержащая числа; строку из файла получаем с помощью getline
    size_t len = 0;
    ssize_t strings=getline(&fileline, &len, fp);  
    while (strings != (ssize_t)-1){			//пока есть строки
        stringnum=strtok(fileline," ");			
        while(stringnum != NULL){			//пока в строке есть числа, разделенные разделителем ' ' 
        	int dot=0;
        	for(int j=0; j<(int)strlen(stringnum)-1;j++){
        		if(stringnum[j]=='.') dot++;			//проверяем вещественное ли число в строке
        		if (!(((stringnum[j]>='0')&&(stringnum[j]<='9'))||(stringnum[j]==' ')||(stringnum[j]=='.'))) goto NEXT;	//смотрим, чтобы в строке было именно вещественное число
        	}
        	if(dot!=1) goto NEXT;    	
        	char *end;
        	float filechislo=strtof(stringnum, &end);	//перевод числа во float
        	if ((filechislo >=(argsrange->b - argsrange->r))&&(filechislo <= argsrange->b + argsrange->r)){		//проверяем, входит ли число в границу
        		ret=0;
        		if (fileline) free(fileline);
        		goto END;
        	}
            NEXT:	
        	stringnum=strtok(NULL," "); 		//получаем новое число для while       
        }
         ret=1;  					//если нет соответствий, то файл не подходит (ret=1)
         strings=getline(&fileline, &len, fp);		//получаем следующую строку для while    
    }
    if (fileline) free(fileline);   
    
    
END:
    if(values) free(values);
    if(argsrange) free(argsrange);			//освобождение памяти, установление ошибок
    fclose(fp); 								
    errno = saved_errno;
    return ret;
    
}


struct input_args * input_check(char *number, struct input_args * args){


   char *float_number;							
   char *end;
   args=(struct input_args *)malloc(sizeof(struct input_args)+1);
   args->b=0;
   args->r=0;
   args->result=1;
   int dogs=0;
   int error=0;
   for(int i=0; i<(int)strlen(number);i++){
   	if ((number[i]=='@')&&(i!=0)){ 
   	    dogs++;
   	    if(number[i+1]=='@'){
	   	 dogs++;
   	    }
   	}
   }
   
   if(dogs!=1){		//если знаков собаки не 1, то ошибка
	error=1;
        goto END;
   }
   
   float_number=strtok(number,"@");		//получаем число до знака собаки и после, разделитель '@'
   int count=0;
   while(float_number!=NULL){
     int dot=0;
     for(int i=0;i<(int)strlen(float_number);i++){
   	if(float_number[i]=='.') dot++;			//поиск точек
      }
      if(dot!=1){		//если указанные числа не типа float( нет точек), то ошибка
	  error=1;
	  goto END;
      }
      if(count==0) args->b=strtof(float_number,&end);	//получение числа типа float
      if(count==1) args->r=strtof(float_number,&end);	//получение числа для области сходимости
      float_number=strtok(NULL,"@");
      count++; 						
   }
   
   
END:   

   if(error){				// проверка на ошибки для возврата информации по строке
   	if(args) free(args);
   	return NULL;
   }
   
   return args;
}


