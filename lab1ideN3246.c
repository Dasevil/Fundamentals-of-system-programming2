#define _GNU_SOURCE         // get_current_dir_name()
#define _XOPEN_SOURCE 500   // nftw()
#define UNUSED(x) (void)(x) //конструкция решает проблему неиспользования аргументов подаваемой функции(nftw аргумент walk_func)
#include <stdlib.h>	    //atexit
#include <string.h>
#include <stdio.h>
#include <ftw.h>
#include <getopt.h>
#include <dlfcn.h>	     //dlopen dlsym
#include <math.h>
#include <dirent.h>	     //opendir readdir
#include <unistd.h>	     //get_current_dir_name() getcwd
#include "plugin_api.h"

struct option *plugins_opt;	//структура определяющая длинную опцию
int *list_dl_plug;						
int lab1ideN3246();		//функция выводa спрaвки по дoступным oпциям
void memory_cleaner();		//функция очищeния памяти по окончaнии прогрaммы
int walk_func(const char*, const struct stat*, int, struct FTW*);	//walk_func с аргументами, соотвествующими структуре рaботы nftw (рeкурсивный поиск)
int plug_totalizer = 0;				//число плагинов(динaмических библиотек)		
struct libscombination {			
    int plugoptions_len;
    char *name;					//структура содержит всю необходимую информацию по динамическим библиотекам, которая используется в работе
    struct plugin_option *plugoptions;  				  
    void *dl;  					
};
int *orderlist_plug= NULL;				//список определяющий порядок опций(заполняется)
struct libscombination *main_libscomb;	
int num_absplugpath = 0;			//проверка числa введeнных путeй до библиoтек
char * program_location;			//абс путь местоположения программы	
struct requirement_opt {     			
  unsigned int not_opt,or_opt,and_opt;	        //список условий, изменяющих соответствие/несоответствие файлов
};
int args_totalizer = 0;				//число аргументов(параметров длинных опций)
char * concatenation(char *path_to, char *fullpath);	//функция склеивания строк в абсолютный путь
char **plugarg_list = NULL;			// список аргументов(параметров длинных опций)
struct requirement_opt requirements = {0, 0, 0};
char *absplugpath;  				


int main(int argc, char *argv[]) {

    if (argc == 1) {		 //когда подается 1 аргумент, то выводим принцип работы программы
        fprintf(stderr, "\tЛабораторная работа номер 1-2 по предмету ОСП\n");
        fprintf(stderr, "Формат работы программы: %s '--opt' 'optarg' 'directory_for_searching'\n", argv[0]);
        fprintf(stderr, "Для получения справки по доступным опциями используйте команду %s -h\n", argv[0]);
        return 1;
    }
    
    char *default_name_dir;			//одно из названий директории, которое не получить
    char *DEBUG = getenv("LAB1DEBUG");        	//debug для доп информации
    atexit(&memory_cleaner);			//выполнение функции по окончании программы для избавления ошибок из-за разного количества free и allocs       
    program_location=getcwd(NULL, 128);		
    absplugpath = get_current_dir_name();	//абс путь к текущей директории
    
    for(int i=0; i < argc; i++) {				
        if (!strcmp(argv[i], "-P")) {			
            if (num_absplugpath) {					//определение опции -P		
                fprintf(stderr, "Ошибка ввода опции %s: максимально допустимое число ввода опции -- 1 \n", argv[i]);	
                return 1;
            }					//проверка на соблюдение условий опции(присутствует аргумент и введена не более 1 раза)
            if (i == (argc - 1)) {
                fprintf(stderr, "Ошибка ввода опции %s: обязательно указывается аргумент (директория для поиска динамических библиотек)\n", argv[i]);		
                return 1;
            }	
            DIR *d;
            if ((d=opendir(argv[i+1])) == NULL) {     		//проверка дирeктoрии на дoступнoсть		
                perror(argv[i+1]);			
                return 1;
            } 
            else {
                free(absplugpath);			
            	closedir(d);					//закрытие директории для избегания ошибок
            	char * plugpatharg=argv[i+1];
            	if (!strncmp(plugpatharg,"./",2)) {			//если директория для библиотек это не абсолютный путь, то получаем aбсoлютный путь, а если aбсолютный, то сохраняем
                    	absplugpath=concatenation(plugpatharg,program_location);
                        } 
                 else {  		
                          absplugpath=argv[i+1];		                         	
                 }	
                num_absplugpath = 1;				
            }   
        }
        else if (strcmp(argv[i],"-h") == 0){  				
                return lab1ideN3246();				//вывод информaции по опциям
        }
        else if (strcmp(argv[i],"-v") == 0) {			
        	printf("Лабораторная работ номер 1-2(основная)\n");	
  		printf("Работу выполнила Ильменская Дарья Евгеньевна\n");
  		printf("Группа N3246\n");
  		printf("Вариант лабораторной работы - 7\n");
  		return 0;
        } 
    }
    char debug_plug_path[80]={0};		//запоминание абсолютного пути
    strcpy(debug_plug_path,absplugpath);			
 
    DIR *d;
    struct dirent *dir;
    d = opendir(absplugpath);
    if (d == NULL) {			//при ошибке opendir возвращает NULL
        perror("opendir");			
        exit(EXIT_FAILURE);
    }
    else{
    	while ((dir = readdir(d)) != NULL) {			
            if ((dir->d_type) == 8) {			//поиск файлов и сумма их количества
                plug_totalizer++;   				
            }							
        }
        closedir(d);
        
    }

	//выделение достаточного количества памяти для каждого найденного плагина)
    main_libscomb = (struct libscombination*)malloc(plug_totalizer*sizeof(struct libscombination));   
    int title=0;				//порядок заполнения
    d = opendir(absplugpath);			
    if (d != NULL) {    			//если открываем без ошибок
        while ((dir = readdir(d)) != NULL) {
            if ((dir->d_type) == 8) {		//ищем файлы
            	if(1){
            	  char *extfile=strrchr(dir->d_name, '.');      	//ищем их расширения в конце после .     	              	  
            	  if(!extfile || extfile==dir->d_name) goto RETRY;	//если неподходит, то идем к следующему
            	  extfile=extfile+1;				
              	  if (!strcmp(extfile,"so")){		
                   	 char absname[240]={0};			//если нашли дин. библиотеку, то получаем абсолютный путь к этому файлу
                    	strcpy(absname, absplugpath);
                    	strcat(absname, "/");
                    	strcat(absname,dir->d_name);				
                   	 void *dl = dlopen(absname, RTLD_LAZY);		//также получаем адрес начала плагина
                   	 if (!dl) {
                    	    fprintf(stderr, "Ошибка: dlopen() провалился(проверьте указанную директорию): %s\n", dlerror());		
                    	    continue;
                    	}
                    	void *func = dlsym(dl, "plugin_get_info");	//и адрес нахождения plugin_get_info			
                    	if (!func) {
                    	    fprintf(stderr, "Ошибка: dlsym() провалился(проверьте указанную директорию): %s\n", dlerror());
                   	 }
                    	struct plugin_info pi = {0};
                    	typedef int (*pgi_func_t)(struct plugin_info*);     
                    	pgi_func_t pgi_func = (pgi_func_t)func;				//используем Вашу структуру и получаем функцию, получающую инфoрмацию по фaйлу, которую перезaписываем	
										
                    	int ret = pgi_func(&pi);
                    	if (ret < 0) {
                      	  fprintf(stderr, "Ошибка: plugin_get_info() провалился(проверьте указанную директорию)\n");
                    	}   
                    	main_libscomb[title].plugoptions = pi.sup_opts;      		          
                    	main_libscomb[title].name = dir->d_name;       		
                    	main_libscomb[title].dl = dl;					//присвoение полученной информации структуре main_libscomb
                    	main_libscomb[title].plugoptions_len = pi.sup_opts_len;
                    	if (DEBUG) {
       		    		printf("DEBUG информация: Обнаружeна динaмичeская библиoтeка:\n  Название библиoтеки: %s\n  Цель библиoтеки: %s\n  Создaтeль(создательницa) библиотeки: %s\n", dir->d_name, pi.plugin_purpose, pi.plugin_author);
                    	}
                    	title++;
                	}
                
              }  
            }
                RETRY:
        }

        closedir(d);
    }

    plug_totalizer=title;
    
    size_t totalizer_opt = 0;
    for(int i = 0; i < plug_totalizer; i++) {			 //вычисляем сумму динамических библиотек
        totalizer_opt=totalizer_opt+main_libscomb[i].plugoptions_len;		
    }

    plugins_opt=(struct option*)malloc(totalizer_opt*sizeof(struct option));	//теперь зная количество, выделяем соответствующее количество пaмяти в программе
    if (!plugins_opt){		//в случае ошибки, завершаем программу
        perror("malloc");		
        exit(EXIT_FAILURE);
    }

    totalizer_opt = 0;
    for(int i = 0; i < plug_totalizer; i++) {				//зaполнeние массива всeх длинных oпций(плагинов) информацией
        for(int j = 0; j < main_libscomb[i].plugoptions_len; j++) {
            plugins_opt[totalizer_opt] = main_libscomb[i].plugoptions[j].opt;		
            totalizer_opt=totalizer_opt+1;
        }
    }


    int proverka;	//проверяем присутствие соответствия введенной длинной опции с доступными для исп.
    for(int i=0; i < argc; i++) {
        if (strstr(argv[i], "--")) {					
            proverka = 0;
            for(size_t j=0; j<totalizer_opt; j++) {
                if (strcmp(&argv[i][2], plugins_opt[j].name) == 0) {	//сравниваем строку после -- в командной строке с именами длинных опций
                    proverka = 1;						
                }
            }
            if (proverka == 0) {						
                fprintf(stderr, "Ошибка: проверьте правильность введенной длинной опции '%s'. Указанная ранее опция не найдена в указанной папке\n", argv[i]);  //оишбка если не существует	
                return 1;
            }
        }
    }

    
    int totalizer_pathdir = 0;
    int ordinal_longsopts = -1;
    int fact_dir=0;
    int c;
    while((c = getopt_long(argc, argv, "-AONP:", plugins_opt, &ordinal_longsopts))!=-1) {				
        switch(c) {										//в зависимости от аргументов командной строки, с помощю getopt_long
        	case 'A':									//мы получим и выведем информацию по тому, что нарушено, чего не хватает...
                	if (!requirements.and_opt) {			//проверка ввода опции 'AND'		
                   		 if (!requirements.or_opt) {			
                       		 requirements.and_opt = 1;
                   	 } 						
                   	 else {						//Нельзя исп. эти опции вместе(логически невозможно)
                        	fprintf(stderr, "Ошибкa комбинировaния опций 'AND' 'OR', выберите что-то одно!\n");
                        	return 1;
                    	 }
                } 
                else {			 //следим за кoличeствoм ввода опции
                    fprintf(stderr, "Ошибкa использовaния опции 'АND': максимально допустимое число использования -- 1\n");
                    return 1;
                }
                break;
                
            case 'O':				//проверка ввода опции 'OR'				
                if (!requirements.or_opt) {					
                    if (!requirements.and_opt) {
                        requirements.or_opt = 1;
                    } 
                    else {				//Нельзя исп. эти опции вместе(логически невозможно)
                        fprintf(stderr, "Ошибкa комбинировaния опций 'AND' 'OR', выберитe что-то oдно!\n");
                        return 1;
                    } 
                } 
                else {			//следим за кoличeствoм ввода опций
                    fprintf(stderr, "Ошибкa использовaния опции 'OR': максимально допустимое число использования -- 1\n");		
                    return 1;
                }
                break;
            
            case 'N':				
                if (!requirements.not_opt){	//проверка ввода опции 'NOT'			
                    requirements.not_opt = 1;
                }
                else{			//следим за кoличeствoм вводa oпции
                    fprintf(stderr, "Ошибкa использовaния опции 'NOT': максимально допустимое число использования -- 1\n");
                    return 1;
                }
                break;
            case 'P':		// опцию P разобрали в самом начале
            	break;

            default:
                if(ordinal_longsopts != -1) {		// значит нашли длинную опцию и узнали её порядок в командной строке		
                    args_totalizer++;			//счет длинныx опций
                    if (DEBUG) {
                        printf("DEBUG: Обнаруженa введенная oпция '%s'. Её аргумeнт: '%s'\n", plugins_opt[ordinal_longsopts].name, optarg); //выводим длинную опцию, что получили
                    }  
                    plugarg_list = (char **) realloc (plugarg_list, args_totalizer * sizeof(char *));   //увеличиваем память для сохранения информации             
                    orderlist_plug = (int*) realloc (orderlist_plug, args_totalizer * sizeof(int));	//увеличиваем память для информации по длинной опции
                    if (!orderlist_plug){
                        perror("realloc");
                        exit(EXIT_FAILURE);
                    }                     
                    plugarg_list[args_totalizer - 1] = optarg;				//записываем аргумент длинной опции
                    orderlist_plug[args_totalizer - 1] = ordinal_longsopts;		//и её порядок
                    ordinal_longsopts = -1;						//устанавливаем значение, которое по умолчанию равно -1, то есть аргументы не длинные опции
                } 
                else {					
                    if (totalizer_pathdir) {		
                        fprintf(stderr, "Ошибкa указaния директории под поиски %s : нельзя указaть две дирeктории для пoиска\n", default_name_dir);	//если директория была уже указана, то завершаем с выводом информации		
                        return 1;
                    }
                    if ((d = opendir(optarg))!= NULL) {   	//проверка opendir директории и получение абсолютного пути путем конкатенации либо присвоения изначального абс.пути                   
                    	if (!strncmp(optarg,"./",2)) {	
                    	        fact_dir++;		
                    		default_name_dir=concatenation(optarg, program_location);
                        } else {  
                        	fact_dir++;
                          	default_name_dir=optarg;
                           }
                        totalizer_pathdir = 1;
                        closedir(d);
                        
                    } else {	
                    	  perror(optarg);
                          return 1;
                    }
                }
        } 
    }

    if (!fact_dir) {		
        fprintf(stderr, "Ошибкa указaния директории для поиска: директория не указана\n");	//узнаем про ввод директории под поиск
        return 1;
    }
    
    if (DEBUG) {				
        printf("DEBUG: Директория с динамичeскими библиотеками-плагинами: %s \n", debug_plug_path); //debug info about debug_plug_path
    }
    
    if (DEBUG) {				
        printf("DEBUG: Дирeктоpия с провeрoчными файлaми: %s \n", default_name_dir );		//debug info о default_name_dir
    }
    
    if (requirements.and_opt==0){			//проверка ввода опции 'AND'
    	if(requirements.or_opt==0){ 
        	requirements.and_opt = 1;
        }			
    }
             
    list_dl_plug = (int *)malloc(args_totalizer*sizeof(int));		
    for(int i=0; i < args_totalizer; i++) {					//ищем соответствие номер-плагин, для однозначного и правильного определения
        const char *names_lonopts = plugins_opt[orderlist_plug[i]].name;			
        for(int j=0; j < plug_totalizer; j++) {
            for(int k=0; k < main_libscomb[j].plugoptions_len; k++) {		
                if (!strcmp(names_lonopts, main_libscomb[j].plugoptions[k].opt.name)) {	//проверка кадлого имени опции   		 
                    list_dl_plug[i] = j;						
                }
            }
        }
    }
    int res = nftw(default_name_dir , walk_func, 40, FTW_PHYS || FTW_DEPTH);		//nftw применяет к каждому файлу walk_func
    if (res != 0) {
        perror("nftw");							//завершение с ошибкой
        return 1;
    }
    
    return 0;
}
   
typedef int (*proc_func_t)( const char *name, struct option in_opts[], size_t in_opts_len);

int walk_func(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {

	char *DEBUG = getenv("LAB1DEBUG");			//для debug
    	if (typeflag == FTW_F) {    				//проверка флага
        int requirements_res = requirements.not_opt^requirements.and_opt; 
        for (int i = 0; i < args_totalizer; i++) {
            struct option opt = plugins_opt[orderlist_plug[i]];
            char * walkfunc_arg = plugarg_list[i];
            if (walkfunc_arg) {
                opt.has_arg = 1;
                opt.flag = (void *)walkfunc_arg;		//получаем аргумент к опции
            } else {
                opt.has_arg = 0;
            }
            
    	    void *func = dlsym(main_libscomb[list_dl_plug[i]].dl, "plugin_process_file");	//конструкция позволяет получить место plugin_process_file в дин.библиотеке
            proc_func_t proc_func = (proc_func_t)func;
            int walkfunc_result;
            
            walkfunc_result = proc_func(fpath, &opt, 1);		//подаем аргументы к плагину и смотрим на результат
            if (walkfunc_result) {	
                if (walkfunc_result > 0) {
                    if(DEBUG){
                       fprintf(stderr, "   Ошибoк работы динамичeской библиотеки не обнаружено\n");		
                    }
                    walkfunc_result = 0;
                } 
                else {
                    fprintf(stderr, "   Обнаружeна ошибкa рабoты динамической библиотeки\n");		
                    return 1;
                }
            } 
            else {
            	if(DEBUG){
                       fprintf(stderr, "   Ошибoк работы динамичeской библиотеки не обнаружено\n");		
                }
                walkfunc_result = 1;
            }
            
          
            if (requirements.not_opt ^ requirements.and_opt) {						
                requirements_res = requirements_res & (requirements.not_opt ^ walkfunc_result);		//логическое получение конечного результата по файлу, учитывая условия 'AND 'NOT 'OR'
                    
            } else {
                requirements_res = requirements_res | (requirements.not_opt ^ walkfunc_result);		//логическое получение конечного результата по файлу, учитывая условия 'AND 'NOT 'OR'
                    
            }
            
        }

        if (requirements_res) {
            printf("  Абсолютный путь до искомого файла: %s\n",fpath);		//printf абс. путь к искомому файлу          
        }
        else{
            if (DEBUG) {
                printf("DEBUG: Абсолютный путь до файла, провалившего проверку: %s\n", fpath);		//printf абс. путь к плохому файлу 
            }
        }
    }	
       
    UNUSED(ftwbuf);		//реализация для избегания ошибок неиспольз. через define 
    UNUSED(sb);   		 //реализация для избегания ошибок неиспольз. через define 
    return 0;
}

char * concatenation(char *path_to, char *fullpath){		//функция склеивания двух строк в абсолютный путь

	char *part_of_link;
	static char masspath[80];
	for(int j=0;j<(int)strlen(fullpath);j++){
		masspath[j]=fullpath[j];
	}
	masspath[(int)strlen(fullpath)]='\0';	
	part_of_link=strchr(path_to,'.')+1;
	strcat(masspath,part_of_link);
	return masspath;

}

void memory_cleaner() {				
 for (int i=0;i<plug_totalizer;i++){					//очищение памяти по окончании выполнения программы
        if (main_libscomb[i].dl) dlclose(main_libscomb[i].dl);
    }
    if (plugins_opt) free(plugins_opt);			
    if (orderlist_plug) free(orderlist_plug);
    if (plugarg_list) free(plugarg_list);
    if (main_libscomb) free(main_libscomb);
    if (list_dl_plug) free(list_dl_plug);
    if (!num_absplugpath) free(absplugpath);
    if (program_location) free(program_location);
    
}


int lab1ideN3246(){
		printf("\t\t\tСправка по доступным опциям к данной работе\n");
		printf("\n");
		printf("-P + directory  Задать каталог с плагинами(обязательно указывается путь).\n");
 		printf("-A      	Объединение опций плагинов с помощью операции «И» (устанавливается по умолчанию).\n");
  		printf("-O      	Объединение опций плагинов с помощью операции «ИЛИ».\n");
  		printf("-N      	Инвертирование условия поиска (после объединения опций плагинов с помощью -A или -O).\n");
  		printf("-v      	Вывод версии программы и информации о программе (ФИО исполнителя, номер группы, номер варианта лабораторной).\n");
 	 	printf("-h      	Вывод всей справки по доступным опциям.\n");
                return 0;
}





