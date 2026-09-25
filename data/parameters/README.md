# Parámetros y notas de diseño

Este directorio reúne las notas `.txt` que estaban dispersas en el repositorio. Se conserva la procedencia en dos subdirectorios para que una tabla pueda rastrearse hasta su ubicación original.

## Mapa de contenidos

| Grupo | Contenido principal |
| --- | --- |
| `directories-work/` | parámetros generales, tablas, estructura y notas de organización |
| `table-important/` | LOX/CH4, propelentes, motores, turbinas, rodamientos, estructura y condiciones de prueba |

Dentro de `table-important/` se conservan los subgrupos `combustion-chamber/`, `fuel-tanks/`, `other-rocket/` y `other-books/` para las notas que estaban anidadas en la fuente.

## Familias de variables detectadas

- **Propulsantes y termoquímica:** LOX, CH4, relaciones de mezcla, propiedades del propelente, temperatura y presión.
- **Cámara y tobera:** garganta, cámara, flujo másico, expansión, Mach y geometría.
- **Alimentación y turbobombas:** caudal, presión, turbinas, rodamientos y condiciones de operación.
- **Estructuras y materiales:** cargas, estructura, UHTC, refrigeración y fabricación.
- **Simulación:** AMReX, NanoVDB, voxelización, CUDA y condiciones de validación.

## Flujo de uso recomendado

1. Identificar la variable y su unidad en la nota original.
2. Registrar la fuente exacta y separar dato publicado, hipótesis y resultado calculado.
3. Convertir los datos a un formato estructurado antes de conectarlos a C++, CUDA, C# o Rust.
4. Validar dimensiones, rangos y sensibilidad antes de alimentar una simulación.

No se han normalizado ni reinterpretado los valores durante esta reorganización. Estas notas no constituyen por sí solas una especificación de fabricación, ignición, ensayo o vuelo.