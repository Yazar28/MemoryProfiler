// main.cpp
#include "Includes/MySimpleLinkedList.h"
#include <iostream>
using namespace std;
using namespace DataStructure;

int main()
{
    // Crear una lista de enteros
    MySimpleList<int> lista;

    // Insertar elementos
    lista.insertar(10);
    lista.insertar(20);
    lista.insertar(30);

    // Mostrar como cola
    cout << "Lista como cola: ";
    lista.mostrarCola();

    // Mostrar como pila
    cout << "Lista como pila: ";
    lista.mostrarPila();

    // Buscar elementos
    cout << "Buscar 20: " << (lista.buscar(20) ? "Encontrado" : "No encontrado") << endl;
    cout << "Buscar 40: " << (lista.buscar(40) ? "Encontrado" : "No encontrado") << endl;

    // Obtener elementos por índice
    cout << "Elemento en índice 0: " << lista.obtener(0) << endl;
    cout << "Elemento en índice 1: " << lista.obtener(1) << endl;
    cout << "Elemento en índice 2: " << lista.obtener(2) << endl;

    // Probar métodos adicionales
    cout << "¿Está vacía? " << (lista.estaVacia() ? "Sí" : "No") << endl;
    cout << "Tamaño: " << lista.tamano() << endl;

    // Eliminar un elemento
    cout << "Eliminando elemento en índice 1..." << endl;
    lista.elimina(1);

    cout << "Lista después de eliminar: ";
    lista.mostrarCola();
    cout << "Nuevo tamaño: " << lista.tamano() << endl;

    // Probar manejo de errores
    try
    {
        cout << "Intentando obtener índice 10: ";
        cout << lista.obtener(10) << endl;
    }
    catch (const out_of_range &e)
    {
        cout << "Error: " << e.what() << endl;
    }

    return 0;
}