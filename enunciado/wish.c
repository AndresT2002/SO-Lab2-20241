
#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <fcntl.h>      
#include <sys/wait.h>   
#include <sys/types.h>  

// Mensaje de error que se debe mostrar en todos los casos de error
char error_message[30] = "An error has occurred\n";

// Variables globales para manejar los directorios donde buscar ejecutables
char **search_paths = NULL;  // Array de strings con los paths
int num_paths = 0;           // Cantidad de paths configurados

// ============================================================================
// FUNCIONES AUXILIARES BÁSICAS
// ============================================================================

/*
 * Imprime el mensaje de error único que se requiere en el laboratorio.
 * Usa write() para escribir directamente a stderr.
 */
void print_error(void) {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

/*
 * Inicializa el shell con el path por defecto.
 * Por defecto solo busca ejecutables en /bin.
 */
void init_shell(void) {
    // Reservar memoria para un solo path 
    search_paths = malloc(sizeof(char*) * 1);
    if (search_paths == NULL) {
        print_error();
        exit(1);
    }
    
    // Configurar /bin como path por defecto para poder acceder a los comandos del sistema (almenos los basicos)
    search_paths[0] = strdup("/bin");
    if (search_paths[0] == NULL) {
        print_error();
        exit(1);
    }
    num_paths = 1;
}

/*
 * Libera toda la memoria usada para almacenar los paths.
 * Se llama al salir del programa para evitar memory leaks.
 */
void free_paths(void) {
    for (int i = 0; i < num_paths; i++) {
        free(search_paths[i]);
    }
    free(search_paths);
    search_paths = NULL;
    num_paths = 0;
}

// ============================================================================
// FUNCIONES PARA MANEJO DE PATHS Y BÚSQUEDA DE EJECUTABLES
// ============================================================================

/*
 * Busca un comando en todos los directorios configurados en el path.
 * Retorna la ruta completa del ejecutable si lo encuentra, NULL si no.
 * 
 * Ejemplo: si busco "ls" y el path es ["/bin", "/usr/bin"], 
 * buscará "/bin/ls" y "/usr/bin/ls" hasta encontrar uno que exista.
 */
char *find_executable(char *cmd) {
    // Si no hay paths configurados, no se puede encontrar nada
    if (num_paths == 0) {
        return NULL;
    }
    
    // Reservar memoria para construir la ruta completa
    char *path = malloc(512);
    if (path == NULL) {
        return NULL;
    }
    
    // Buscar en cada directorio del path
    for (int i = 0; i < num_paths; i++) {
        // Construir ruta completa: "/bin/ls"
        snprintf(path, 512, "%s/%s", search_paths[i], cmd);
        
        // Verificar si el archivo existe y es ejecutable
        if (access(path, X_OK) == 0) {
            return path;  // Si se encuentra el ejecutable, se retorna la ruta completa
        }
    }
    
    // No se encontró en ningún directorio
    free(path);
    return NULL;
}

// ============================================================================
// FUNCIONESSS PARA COMANDOS BUILT-IN
// ============================================================================

/*
 * Verifica si un comando es built-in (implementado por el shell).
 * Los comandos built-in son: exit, cd, path
 */
int is_builtin(char *cmd) {
    if (cmd == NULL) return 0;
    if (strcmp(cmd, "exit") == 0) return 1;
    if (strcmp(cmd, "cd") == 0) return 1;
    if (strcmp(cmd, "path") == 0) return 1;
    return 0;
}

/*
 * Ejecuta un comando built-in. Los comandos built-in se ejecutan directamente
 * en el proceso del shell (no se crea un proceso hijo).
 */
void execute_builtin(char **args, int arg_count) {
    if (strcmp(args[0], "exit") == 0) {
        // Comando exit: salir del shell
        if (arg_count > 1) {
            print_error();  // Error: exit no debe tener argumentos
            return;
        }
        // Limpiar memoria y salir
        free_paths();
        exit(0);
    }
    else if (strcmp(args[0], "cd") == 0) {
        // Comando cd: cambiar directorio
        if (arg_count != 2) {
            print_error();  // Error: cd debe tener exactamente 1 argumento
            return;
        }
        if (chdir(args[1]) != 0) {
            print_error();  // Error: no se pudo cambiar al directorio
        }
    }
    else if (strcmp(args[0], "path") == 0) {
        // Comando path: configurar directorios de búsqueda
        // Primero liberar los paths anteriores
        free_paths();
        
        if (arg_count == 1) {
            // path sin argumentos = path vacío (solo comandos built-in funcionan)
            num_paths = 0;
            search_paths = NULL;
        } else {
            // Configurar nuevos paths con los argumentos dados
            num_paths = arg_count - 1;  // -1 porque args[0] es "path"
            search_paths = malloc(sizeof(char*) * num_paths);
            if (search_paths == NULL) {
                print_error();
                exit(1);
            }
            
            // Copiar cada path
            for (int i = 0; i < num_paths; i++) {
                search_paths[i] = strdup(args[i + 1]);
                if (search_paths[i] == NULL) {
                    print_error();
                    exit(1);
                }
            }
        }
    }
}

// ============================================================================
// FUNCIONES PARA EJECUCIÓN DE COMANDOS EXTERNOS
// ============================================================================

/*
 * Ejecuta un comando externo (no built-in) creando un proceso hijo.
 * También maneja la redirección de salida si se especifica un archivo.
 */
void execute_external(char **args, char *redirect_file) {
    if (args[0] == NULL) {
        return;
    }
    
    // Buscar el ejecutable en los directorios del path
    char *executable = find_executable(args[0]);
    if (executable == NULL) {
        print_error();  // Error: comando no encontrado
        return;
    }
    
    // Crear proceso hijo
    pid_t pid = fork();
    if (pid < 0) {
        // Error al crear proceso hijo
        print_error();
        free(executable);
        return;
    }
    else if (pid == 0) {
        // PROCESO HIJO: ejecutar el comando
        
        // Si hay redirección, configurarla antes de ejecutar
        if (redirect_file != NULL) {
            // Abrir archivo para escritura (crear si no existe, truncar si existe)
            int fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                print_error();
                exit(1);
            }
            // Redirigir stdout y stderr al archivo (ambos al mismo archivo)
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        
        // Ejecutar el comando (si execv retorna, hubo error)
        if (execv(executable, args) == -1) {
            print_error();
            exit(1);
        }
    }
    else {
        // PROCESO PADRE: esperar a que termine el hijo
        free(executable);
        wait(NULL);
    }
}

// ============================================================================
// FUNCIONES DE PARSING Y EJECUCIÓN
// ============================================================================

/*
 * Parsea y ejecuta un comando simple (sin comandos paralelos &).
 * Maneja la redirección de salida si está presente.
 */
void parse_and_execute_single(char *cmd_str) {
    // Reservar memoria para los argumentos del comando
    char **args = malloc(sizeof(char*) * 512);
    if (args == NULL) {
        print_error();
        return;
    }
    
    int arg_count = 0;           // Contador de argumentos
    char *redirect_file = NULL;  // Archivo de redirección (si existe)
    int redirect_count = 0;      // Contador de símbolos >
    
    // Hacer una copia de la línea para poder tokenizarla
    char *cmd_copy = strdup(cmd_str);
    if (cmd_copy == NULL) {
        print_error();
        free(args);
        return;
    }
    
    // Tokenizar la línea separando por espacios y tabs es decir se crea un array de strings donde cada string es un token
    char *token = strtok(cmd_copy, " \t");
    while (token != NULL && arg_count < 511) {
        if (strcmp(token, ">") == 0) {
            // Encontramos redirección
            redirect_count++;
            
            // El siguiente token debe ser el archivo de destino
            token = strtok(NULL, " \t");
            if (token == NULL) {
                // Error: no hay archivo después de >
                print_error();
                free(cmd_copy);
                free(args);
                return;
            }
            
            // Guardar el nombre del archivo
            if (redirect_file != NULL) {
                free(redirect_file);
            }
            redirect_file = strdup(token);
            
            // Verificar que no haya más tokens después del archivo
            token = strtok(NULL, " \t");
            if (token != NULL && strcmp(token, ">") != 0) {
                // Error: hay tokens después del archivo de redirección
                print_error();
                free(cmd_copy);
                free(redirect_file);
                for (int i = 0; i < arg_count; i++) {
                    free(args[i]);
                }
                free(args);
                return;
            }
            continue;  // Saltar al siguiente token
        }
        
        // Es un argumento normal del comando
        args[arg_count] = strdup(token);
        if (args[arg_count] == NULL) {
            print_error();
            // Limpiar memoria ya asignada
            for (int i = 0; i < arg_count; i++) {
                free(args[i]);
            }
            free(args);
            free(cmd_copy);
            if (redirect_file) free(redirect_file);
            return;
        }
        arg_count++;
        token = strtok(NULL, " \t");
    }
    args[arg_count] = NULL;  // Marcar el final del array
    
    free(cmd_copy);
    
    // Validar errores de redirección
    if (redirect_count > 1) {
        print_error();  // Error: múltiples símbolos >
        for (int i = 0; i < arg_count; i++) {
            free(args[i]);
        }
        free(args);
        if (redirect_file) free(redirect_file);
        return;
    }
    
    if (arg_count == 0) {
        // Error: no hay comando (solo > archivo)
        if (redirect_file != NULL) {
            print_error();
            free(redirect_file);
        }
        free(args);
        return;
    }
    
    // Ejecutar el comando (built-in o externo)
    if (is_builtin(args[0])) {
        execute_builtin(args, arg_count);
    } else {
        execute_external(args, redirect_file);
    }
    
    // Limpiar memoria
    for (int i = 0; i < arg_count; i++) {
        free(args[i]);
    }
    free(args);
    if (redirect_file) free(redirect_file);
}

/*
 * Función principal de parsing. Maneja tanto comandos simples como paralelos.
 * Detecta si hay símbolos & y ejecuta los comandos en paralelo si es necesario.
 */
void parse_and_execute(char *line) {
    // Limpiar la línea: remover salto de línea al final para poder procesarla sin un problema
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
    
    // Verificar si la línea está vacía o solo tiene espacios/tabs
    len = strlen(line);
    int only_spaces = 1;
    for (size_t i = 0; i < len; i++) {
        if (line[i] != ' ' && line[i] != '\t') {
            only_spaces = 0;
            break;
        }
    }
    if (len == 0 || only_spaces) {
        return;  // Ignorar líneas vacías por si el usuario no ingresa nada, no hacer nada
    }
    
    // Hacer una copia para poder manipularla
    char *line_copy = strdup(line);
    if (line_copy == NULL) {
        print_error();
        return;
    }
    
    // Contar cuántos comandos hay (separados por &), estos son lo qe se llaman comandos paralelos 
    int num_commands = 1;
    for (size_t i = 0; i < strlen(line_copy); i++) {
        if (line_copy[i] == '&') {
            num_commands++;
        }
    }
    
    //Acá empieza la lógica para saber como ejecutar los comandos sea 1 o varios (paralelos)
    if (num_commands == 1) {
        // Comando simple sin paralelismo
        parse_and_execute_single(line_copy);
        free(line_copy);
    } else {
        // COMANDOS PARALELOS (más de 1 comando): ejecutar todos al mismo tiempo
        
        // Array para guardar los PIDs de los procesos hijos, donde el pid es el identificador único de cada proceso
        pid_t *pids = malloc(sizeof(pid_t) * num_commands);
        if (pids == NULL) {
            print_error();
            free(line_copy);
            return;
        }
        
        int cmd_index = 0;
        char *cmd = strtok(line_copy, "&");
        
        // Crear un proceso hijo para cada comando
        while (cmd != NULL && cmd_index < num_commands) {
            pid_t pid = fork();
            if (pid < 0) {
                // Error al crear proceso hijo
                print_error();
                // Esperar a los hijos ya creados antes de salir
                for (int i = 0; i < cmd_index; i++) {
                    waitpid(pids[i], NULL, 0);
                }
                free(pids);
                free(line_copy);
                return;
            } else if (pid == 0) {
                // PROCESO HIJO: ejecutar este comando específico
                parse_and_execute_single(cmd);
                exit(0);  // El hijo termina aquí
            } else {
                // PROCESO PADRE: guardar el PID del hijo
                pids[cmd_index] = pid;
                cmd_index++;
            }
            cmd = strtok(NULL, "&");  // Siguiente comando
        }
        
        // Esperar a que TODOS los procesos hijos terminen
        for (int i = 0; i < cmd_index; i++) {
            waitpid(pids[i], NULL, 0);
        }
        
        free(pids);
        free(line_copy);
    }
}

// ============================================================================
// PROGRAMA PRINCIPAL
// ============================================================================

/*
 * FUNCIÓN MAIN - este es el inicioo del programa
 * 
 * Esta función es el corazón del shell. Su flujo es:
 * 1. Verificar argumentos de línea de comandos
 * 2. Configurar modo interactivo o batch
 * 3. Inicializar el shell
 * 4. Loop principal: leer -> parsear -> ejecutar -> repetir
 * 5. Limpiar recursos y salir
 */
int main(int argc, char *argv[]) {
    FILE *input = stdin;        // Por defecto lee de la consola
    int interactive_mode = 1;   // Por defecto modo interactivo
    
    // PASO 1: Verificar argumentos de línea de comandos
    // El shell acepta 0 argumentos (modo interactivo) o 1 argumento (archivo batch), donde el argc es el número de argumentos de línea de comandos
    if (argc > 2) {
        print_error();  // Error: demasiados argumentos
        exit(1);
    }
    
    // PASO 2: Configurar modo de operación
    if (argc == 2) {
        // Modo batch: leer comandos desde un archivo, donde el fopen abre el archivo en modo lectura
        input = fopen(argv[1], "r");
        if (input == NULL) {
            print_error();  // Error: no se pudo abrir el archivo
            exit(1);
        }
        interactive_mode = 0;  // No mostrar prompt en modo batch
    }
    
    // PASO 3: Inicializar el shell (se llama la función encargada de eso)
    init_shell();  // Configurar path por defecto (/bin)
    
    // Variables para leer líneas de entrada
    char *line = NULL;      // Buffer para la línea
    size_t linecap = 0;     // Capacidad del buffer
    ssize_t linelen;        // Longitud de la línea leída
    
    // PASO 4: LOOP PRINCIPAL DEL SHELL
    // Este es el ciclo infinito que hace que el shell funcione
    while (1) {
        // Mostrar prompt solo en modo interactivo
        if (interactive_mode) {
            printf("wish> ");
            fflush(stdout);  // Asegurar que se muestre inmediatamente
        }
        
        // Leer una línea de entrada (del usuario o del archivo)
        linelen = getline(&line, &linecap, input);
        
        // Si llegamos al final del archivo (EOF), salir del shell
        if (linelen == -1) {
            break;
        }
        
        // Procesar y ejecutar la línea de comando
        parse_and_execute(line);
    }
    
    // PASO 5: LIMPIEZA Y SALIDA
    // Liberar toda la memoria antes de terminar
    if (line != NULL) {
        free(line);
    }
    if (!interactive_mode) {
        fclose(input);  // Cerrar archivo si estaba en modo batch
    }
    free_paths();  // Liberar memoria de los paths
    
    return 0;  // Salir exitosamente
}
