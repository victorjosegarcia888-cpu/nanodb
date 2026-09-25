# Datos normalizados

`lox_ch4_concept_reference.yaml` es el primer contrato de caso para conectar notas con C++, CUDA, AMReX y geometría. Sus valores se han transcrito de los `.txt` indicados en `source_notes`, con unidades SI cuando la fuente permite determinarlas.

## Estado de confianza

- `reference_only` significa que el caso sirve para pruebas de lectura, validación de unidades y geometría conceptual.
- No significa que los valores formen una especificación de motor.
- Los caudales, la química, las propiedades criogénicas y las condiciones de contorno deben reconciliarse antes de una simulación reactiva.
- En particular, la nota de parámetros contiene caudales cuya relación no coincide automáticamente con el O/F de la tabla termoquímica; esa discrepancia queda pendiente y no se corrige aquí de forma silenciosa.

La extracción de contenido PDF queda pendiente de una herramienta verificable (`pdftotext`, `mutool` o una librería equivalente). Mientras tanto, los PDFs se usan como referencias catalogadas por nombre, no como una fuente numérica ya parseada.