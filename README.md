# Instrucciones de Ejecución

Este documento proporciona los pasos necesarios para compilar y ejecutar los componentes del proyecto: Bowman, Poole y Discovery.

## Compilación

1. **Compilar el proyecto:**
   En el directorio raíz del proyecto, donde se encuentra el `makefile`, ejecutar el comando:
    ```bash
   make
   ```

   **Nota:** Los archivos `.o` se eliminarán automáticamente después de la compilación.

2. **Conversión de formato de archivos de configuración:**
Asegúrate de que el formato de los archivos de configuración sea Unix y no Windows. Para ello, ejecuta el siguiente comando en el mismo nivel que el `makefile`:
    ```bash
   dos2unix

## Ejecución con Valgrind

Para ejecutar los componentes con Valgrind y analizar posibles fugas de memoria, sigue estos pasos:

1. **Ejecutar Discovery:**
    ```bash
    cd Discovery
    valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes -s --track-origins=yes ./discovery ../d.dat

2. **Ejecutar Poole:**
    ```bash
    cd Poole
    valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes -s --track-origins=yes ./poole ../p.dat

3. **Ejecutar Bowman:**
    ```bash
    cd Bowman
    valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes -s --track-origins=yes ./bowman ../b.dat

## Información Adicional

- Los ficheros de configuración relacionados con Bowman empiezan con "b".
- Los ficheros de configuración relacionados con Poole empiezan con "p".
- El fichero de configuración relacionado con Discovery empieza con "d".
