# StarWarsText

Aplicacion Qt/C++ para editar y mostrar un texto animado con una presentacion inspirada en el opening crawl de Star Wars.

## Estado actual

- La aplicacion arranca directamente en modo `Show`.
- El modo `Show` se abre en ventana normal por defecto.
- `F11` alterna entre ventana normal y pantalla completa.
- `Esc` sale del modo `Show` y vuelve al editor.
- El editor permite modificar el contenido de `resources/text.txt`.
- `Ctrl+S` guarda el texto editado.
- Al cerrar la aplicacion con cambios sin guardar, se ofrece `Save`, `Discard` o `Cancel`.

## Formato del texto

El contenido mostrado se toma de [resources/text.txt](C:/Users/caico/Workspaces/C++/StarWarsText/resources/text.txt).

Reglas actuales:

- La primera linea no vacia es el titulo.
- La segunda linea no vacia es el subtitulo.
- A partir de ahi, el resto de lineas se interpreta como cuerpo.
- Los saltos de linea del cuerpo se respetan tal cual.
- Las lineas en blanco del cuerpo se usan como separadores de parrafo.

Ejemplo:

```text
TITLE

SUBTITLE

This is the first line of the
first paragraph. We will have
more lines than this, I think
so...

And this is the first line of
the second paragraph. We are
going to have more lines too
```

## Interfaz

- `Show`: vuelve a lanzar la visualizacion con el contenido actual del editor.
- `Ctrl+S`: guarda el archivo editable.
- `Esc`: desde el crawl, vuelve al editor.
- `F11`: alterna fullscreen en el crawl.

## Implementacion actual

- Proyecto Qt 6 Widgets con CMake.
- El texto por defecto tambien esta embebido como recurso Qt mediante `resources/resources.qrc`.
- El crawl usa un fondo estrellado generado por codigo.
- El texto se prerenderiza en una imagen y despues se proyecta en perspectiva para conseguir el efecto de alejamiento.
- La velocidad del scroll se adapta al tamano de la ventana para que el comportamiento sea razonable tanto en ventana como en fullscreen.

## Build

Requisitos:

- Qt 6.7.3 para MSVC 2022 en `C:/Qt/6.7.3/msvc2022_64`
- CMake
- Compilador MSVC compatible con C++20

Archivos principales:

- [CMakeLists.txt](C:/Users/caico/Workspaces/C++/StarWarsText/CMakeLists.txt)
- [main.cpp](C:/Users/caico/Workspaces/C++/StarWarsText/main.cpp)
- [resources/resources.qrc](C:/Users/caico/Workspaces/C++/StarWarsText/resources/resources.qrc)
- [resources/text.txt](C:/Users/caico/Workspaces/C++/StarWarsText/resources/text.txt)

## Siguientes pasos

- Limpiar y simplificar `main.cpp`, separando editor, crawl y logica de carga/guardado.
- Extraer la configuracion visual del crawl a constantes o una estructura dedicada para facilitar el ajuste.
- Modelar titulo, subtitulo y cuerpo con una estructura mas explicita en lugar de depender solo de posiciones de linea.
