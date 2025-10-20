# Informe Laboratorio 2: Implementación de Shell Unix (wish)

### Andrés Guillermo Toloza Guzman 1004843452

### Jair Santiago Leal Miranda 1000293157

##

Este documento presenta la implementación de un shell Unix básico denominado `wish` (Wisconsin Shell), desarrollado en lenguaje C. El shell implementa las funcionalidades fundamentales de un intérprete de comandos, incluyendo ejecución de comandos externos, comandos integrados, redirección de salida y ejecución paralela de comandos.

## 1. Introducción

Un shell es un intérprete de comandos que actúa como interfaz entre el usuario y el sistema operativo. La implementación desarrollada simula el comportamiento de shells Unix tradicionales como bash, pero con funcionalidades simplificadas

## 2. Arquitectura del Sistema

### 2.1 Estructura General

El programa está estructurado en módulos funcionales que manejan diferentes aspectos del shell:

- **Gestión de memoria**: Manejo dinámico de paths y argumentos
- **Parsing**: Análisis sintáctico de comandos de entrada
- **Ejecución**: Procesamiento de comandos integrados y externos ( es decir, revisar que ese comando esté permitido en nuestra implementación)
- **Redirección**: Manejo de entrada/salida hacia archivos

### 2.2 Flujo Principal

El flujo de ejecución sigue el patrón clásico de shells Unix:

1. **Inicialización**: Configuración del path por defecto (`/bin`)
2. **Loop principal**: Lectura continua de comandos
3. **Parsing**: Análisis sintáctico de la entrada
4. **Ejecución**: Procesamiento del comando
5. **Limpieza**: Liberación de recursos

## 3. Funcionalidades Implementadas

### 3.1 Comandos Integrados (Built-in Commands)

Se implementaron tres comandos integrados que se ejecutan directamente en el proceso del shell:

- **`exit`**: Termina la ejecución del shell
- **`cd`**: Cambia el directorio de trabajo actual
- **`path`**: Configura los directorios de búsqueda de ejecutables

### 3.2 Comandos Externos

Los comandos externos se ejecutan mediante la creación de procesos hijos utilizando las llamadas al sistema `fork()` y `execv()`. El shell busca los ejecutables en los directorios configurados en el path de búsqueda.

### 3.3 Redirección de Salida

Se implementó redirección de salida estándar y de error estándar hacia archivos utilizando el operador `>`. La implementación redirige tanto `stdout` como `stderr` al archivo especificado.

### 3.4 Comandos Paralelos

El shell soporta ejecución paralela de comandos mediante el operador `&`. Cuando se detectan múltiples comandos separados por `&`, se crean procesos hijos para cada comando y se ejecutan concurrentemente.

## 4. Modos de Operación

### 4.1 Modo Interactivo

En modo interactivo, el shell presenta el prompt `wish> ` y espera comandos del usuario a través de la entrada estándar.

### 4.2 Modo Batch

En modo batch, el shell lee comandos desde un archivo especificado como argumento de línea de comandos, sin mostrar prompt.

## 5. Gestión de Memoria

La implementación utiliza gestión dinámica de memoria con las siguientes consideraciones:

- **Paths de búsqueda**: Array dinámico de strings para almacenar directorios
- **Argumentos de comandos**: Arrays dinámicos para argumentos de comandos
- **Limpieza**: Liberación sistemática de memoria al finalizar

## 6. Manejo de Errores

Se implementó un sistema unificado de manejo de errores que utiliza un mensaje estándar: `"An error has occurred\n"`. Todos los errores se reportan a través de `stderr` utilizando la llamada al sistema `write()`.

## 7. Consideraciones Técnicas

### 7.1 Llamadas al Sistema Utilizadas

- `fork()`: Creación de procesos hijos
- `execv()`: Ejecución de programas externos
- `wait()/waitpid()`: Sincronización con procesos hijos
- `chdir()`: Cambio de directorio
- `access()`: Verificación de permisos de archivos
- `open()/dup2()`: Redirección de entrada/salida

### 7.2 Parsing y Tokenización

El análisis sintáctico se realiza mediante `strtok()` para separar argumentos y detectar operadores especiales (`>`, `&`). Se implementó validación para manejar espacios en blanco y casos de error.

## 8. Resultados y Validación

La implementación cumple con los requisitos especificados:

- Ejecución de comandos externos
- Comandos integrados (exit, cd, path)
- Redirección de salida
- Comandos paralelos
- Modos interactivo y batch
- Manejo robusto de errores

## 9. Conclusiones

La implementación del shell `wish` demuestra la comprensión de conceptos fundamentales de sistemas operativos, incluyendo:

- Gestión de procesos mediante `fork()` y `exec()`
- Comunicación entre procesos
- Manejo de entrada/salida del sistema
- Gestión de memoria dinámica
- Parsing y análisis sintáctico
