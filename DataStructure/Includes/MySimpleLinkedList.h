// Bases_de_datos.h
#ifndef BASES_DE_DATOS_H
#define BASES_DE_DATOS_H
using namespace std;
#include <iostream>
namespace DataStructure
{
    template <typename T>
    class MySimpleListNodo
    {
    public:
        T Valor;
        MySimpleListNodo<T> *Next;

        MySimpleListNodo(T Valor);
        ~MySimpleListNodo();
    };

    template <typename T>
    class MySimpleList
    {
    private:
        MySimpleListNodo<T> *head;

    public:
        MySimpleList();
        ~MySimpleList();
        void insertar(T Valor);
        void mostrarCola();
        void mostrarPila();
        bool buscar(T Valor);
        T obtener(int indice);
        void elimina(int indice);

        // Métodos adicionales útiles
        bool estaVacia() const;
        int tamano() const;

    private:
        void insertarRecursivo(MySimpleListNodo<T> *&nodo, T Valor);
        bool buscarRecursivo(MySimpleListNodo<T> *nodo, T Valor);
        T obtenerRecursivo(MySimpleListNodo<T> *nodo, int indice, int actual);
        void mostrarPilaRecursivo(MySimpleListNodo<T> *nodo);
        void mostrarColaRecursivo(MySimpleListNodo<T> *nodo);
        void eliminaRecursivo(MySimpleListNodo<T> *&nodo, int indice, int actual);
    };

    // Implementación de MySimpleListNodo
    template <typename T>
    MySimpleListNodo<T>::MySimpleListNodo(T Valor)
    {
        this->Valor = Valor;
        this->Next = nullptr;
    }
    template <typename T>
    MySimpleListNodo<T>::~MySimpleListNodo()
    {
        delete Next;
    }

    // Implementación de MySimpleList
    template <typename T>
    MySimpleList<T>::MySimpleList()
    {
        head = nullptr;
    }
    template <typename T>
    MySimpleList<T>::~MySimpleList()
    {
        delete head;
    }
    template <typename T>
    void MySimpleList<T>::insertar(T Valor)
    {
        insertarRecursivo(head, Valor);
    }
    template <typename T>
    void MySimpleList<T>::mostrarCola()
    {
        mostrarColaRecursivo(head);
        std::cout << std::endl;
    }
    template <typename T>
    void MySimpleList<T>::mostrarPila()
    {
        mostrarPilaRecursivo(head);
        std::cout << std::endl;
    }
    template <typename T>
    bool MySimpleList<T>::buscar(T Valor)
    {
        return buscarRecursivo(head, Valor);
    }
    template <typename T>
    T MySimpleList<T>::obtener(int indice)
    {
        return obtenerRecursivo(head, indice, 0);
    }
    template <typename T>
    void MySimpleList<T>::elimina(int indice)
    {
        eliminaRecursivo(head, indice, 0);
    }
    template <typename T>
    void MySimpleList<T>::insertarRecursivo(MySimpleListNodo<T> *&nodo, T Valor)
    {
        if (nodo == nullptr)
        {
            nodo = new MySimpleListNodo<T>(Valor);
        }
        else
        {
            insertarRecursivo(nodo->Next, Valor);
        }
    }
    template <typename T>
    void MySimpleList<T>::mostrarColaRecursivo(MySimpleListNodo<T> *nodo)
    {
        if (nodo != nullptr)
        {
            std::cout << nodo->Valor << " ";
            mostrarColaRecursivo(nodo->Next);
        }
    }
    template <typename T>
    void MySimpleList<T>::mostrarPilaRecursivo(MySimpleListNodo<T> *nodo)
    {
        if (nodo != nullptr)
        {
            mostrarPilaRecursivo(nodo->Next);
            std::cout << nodo->Valor << " ";
        }
    }
    template <typename T>
    bool MySimpleList<T>::buscarRecursivo(MySimpleListNodo<T> *nodo, T Valor)
    {
        if (nodo == nullptr)
        {
            return false;
        }
        else if (nodo->Valor == Valor)
        {
            return true;
        }
        else
        {
            return buscarRecursivo(nodo->Next, Valor);
        }
    }
    template <typename T>
    T MySimpleList<T>::obtenerRecursivo(MySimpleListNodo<T> *nodo, int indice, int actual)
    {
        if (nodo == nullptr)
        {
            throw std::out_of_range("Índice fuera de rango");
        }
        else if (actual == indice)
        {
            return nodo->Valor;
        }
        else
        {
            return obtenerRecursivo(nodo->Next, indice, actual + 1);
        }
    }
    template <typename T>
    void MySimpleList<T>::eliminaRecursivo(MySimpleListNodo<T> *&nodo, int indice, int actual)
    {
        if (nodo == nullptr)
        {
            throw std::out_of_range("Índice fuera de rango");
        }
        else if (actual == indice)
        {
            MySimpleListNodo<T> *temp = nodo;
            nodo = nodo->Next;
            temp->Next = nullptr;
            delete temp;
        }
        else
        {
            eliminaRecursivo(nodo->Next, indice, actual + 1);
        }
    }

    // Métodos adicionales útiles
    template <typename T>
    bool MySimpleList<T>::estaVacia() const
    {
        return head == nullptr;
    }
    template <typename T>
    int MySimpleList<T>::tamano() const
    {
        int count = 0;
        MySimpleListNodo<T> *nodo = head;
        while (nodo != nullptr)
        {
            count++;
            nodo = nodo->Next;
        }
        return count;
    }

} // namespace DataStructure

#endif