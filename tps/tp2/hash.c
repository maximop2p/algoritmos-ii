#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE 1
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "hash.h"
#define CANT_PRIMOS 14
static const unsigned long primos[CANT_PRIMOS] = {47, 89, 167, 337, 673, 1319, 2677, 5107, 10211, 20431, 40867, 81611, 163169, 326503};

typedef enum estado {
    VACIO,
    OCUPADO,
    BORRADO
} estado_t;

typedef struct hash_elem {
    char* clave;
    void* dato;
    estado_t estado;
} hash_elem_t;

struct hash {
    hash_elem_t* tabla;
    size_t capacidad, cantidad, contador_primos, borrados;
    hash_destruir_dato_t destruir_dato;
};

size_t hash_proximo(const hash_t* hash, size_t pos_actual);
bool hash_redimensionar(hash_t* hash);
size_t hash_buscar_clave(const hash_t* hash, const char* clave, size_t pos, bool* existia);

// Vamos a usar djb2
// fuente: http://www.cse.yorku.ca/~oz/hash.html#djb2

unsigned long f_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

hash_t *hash_crear(hash_destruir_dato_t destruir_dato) {
    hash_t* h = calloc(1, sizeof(hash_t));
    if (!h) return NULL;
    h->contador_primos = 0;
    h->borrados = 0;
    h->tabla = calloc(primos[h->contador_primos], sizeof(hash_elem_t));
    if (!h->tabla) {
        free(h);
        return NULL;
    }
    h->capacidad = primos[h->contador_primos];
    h->destruir_dato = destruir_dato;
    return h;
}

bool hash_guardar(hash_t *hash, const char *clave, void *dato) {
    if ((((float)hash->cantidad + (float)hash->borrados)/ (float)hash->capacidad) >= 0.7)  // cuando el factor de carga es mayor a 0.7, redimensiono.
        if (!hash_redimensionar(hash))
            return false;
    bool existia;
    size_t pos = hash_buscar_clave(hash, clave, f_hash(clave) % hash->capacidad, &existia);
    if (existia) {
        if (hash->destruir_dato)
            hash->destruir_dato(hash->tabla[pos].dato);
        hash->tabla[pos].dato = dato;
    } else {
        hash->tabla[pos].dato = dato;
        hash->tabla[pos].clave = strdup(clave);
        hash->tabla[pos].estado = OCUPADO;
        hash->cantidad++;
    }
    return true;
}

void *hash_borrar(hash_t *hash, const char *clave) {
    bool pertenece;
    size_t pos = hash_buscar_clave(hash, clave, f_hash(clave) % hash->capacidad, &pertenece);
    if (!pertenece) return NULL;
    void* dato = hash->tabla[pos].dato;
    free(hash->tabla[pos].clave);
    hash->tabla[pos].estado = BORRADO;
    hash->tabla[pos].dato = NULL;
    hash->cantidad--;
    hash->borrados++;
    return dato;
}

void *hash_obtener(const hash_t *hash, const char *clave) {
    bool pertenece;
    size_t pos = hash_buscar_clave(hash, clave, f_hash(clave) % hash->capacidad, &pertenece);
    if (!pertenece) return NULL;
    return hash->tabla[pos].dato;
}

bool hash_pertenece(const hash_t *hash, const char *clave) {
    bool pertenece;
    hash_buscar_clave(hash, clave, f_hash(clave) % hash->capacidad, &pertenece);
    return pertenece;
}

size_t hash_cantidad(const hash_t *hash) {
    return hash->cantidad;
}

void hash_destruir(hash_t *hash) {
    for (size_t i = 0; i < hash->capacidad;  i++) {
        if (hash->tabla[i].estado == OCUPADO) {
            free(hash->tabla[i].clave);
            if (hash->destruir_dato)
                hash->destruir_dato(hash->tabla[i].dato);
        }
    }
    free(hash->tabla);
    free(hash);
}

// Primitivas - iterador del hash
struct hash_iter {
    const hash_t* hash;
    size_t actual, recorridos;
};

hash_iter_t *hash_iter_crear(const hash_t *hash) {
    hash_iter_t* iter = malloc(sizeof(hash_iter_t));
    if (!iter) return NULL;
    iter->hash = (const hash_t*)hash;
    if(hash->tabla[0].estado == OCUPADO){
        iter->actual = 0;
    }
    else{
        iter->actual = hash_proximo(hash, 0);
    }
    iter->recorridos = 0;
    return iter;
}

bool hash_iter_avanzar(hash_iter_t *iter) {
    if (hash_iter_al_final(iter)) return false;
    size_t prox = hash_proximo(iter->hash, iter->actual);
    iter->actual = prox;
    iter->recorridos++;
    return true;
}

const char *hash_iter_ver_actual(const hash_iter_t *iter) {
    if (hash_iter_al_final(iter)) return NULL;
    return iter->hash->tabla[iter->actual].clave;
}

bool hash_iter_al_final(const hash_iter_t *iter) {
    return iter->hash->cantidad == iter->recorridos;
}

void hash_iter_destruir(hash_iter_t *iter) {
    free(iter);
}

// Redimensiona la tabla de hash, cambiando su capacidad a "nuevo_tam".
// Devuelve false en caso de haber algun error con la memoria.
// Devuelve true si sale todo bien.
// Pre: Recibe un hash inicializado.
// Post: La tabla de hash redimensiono su capacidad a "nuevo_tam", y se asignaron nuevos lugares para cada clave (teniendo en cuenta la nueva capacidad)
bool hash_redimensionar(hash_t* hash) {
    hash->contador_primos++;
    size_t capacidad_post;
    if(hash->contador_primos >= CANT_PRIMOS){
        capacidad_post = (hash->capacidad) * 2;
    }
    else{
        capacidad_post = primos[hash->contador_primos];
    }
    hash_elem_t* tabla_post = calloc(capacidad_post, sizeof(hash_elem_t));
    if (!tabla_post) {
        hash->contador_primos--;
        return false;
    }

    hash_elem_t* tabla_pre = hash->tabla;
    size_t capacidad_pre = hash->capacidad;

    hash->tabla = tabla_post;
    hash->capacidad = capacidad_post;
    hash->cantidad = 0;

    for (size_t i = 0; i < capacidad_pre; i++) {
        if (tabla_pre[i].estado == OCUPADO) {
            hash_guardar(hash, tabla_pre[i].clave, tabla_pre[i].dato);
            free(tabla_pre[i].clave);
        }
    }
    free(tabla_pre);
    return true;
}


// Recibe un hash y la posicion actual en la que se encuentra.
// Devuelve la posicion del proximo elemento en la tabla.
// En caso de llegar al final de la tabla y no encontrar un nuevo elemento, devuelve la misma posicion actual.
// Pre: Recibe un hash inicializado, y una posicion actual valida.
// Post: Devuelve la posicion en la que se encuentra el proximo elemento de la tabla.
// En caso de no encontrar un proximo elemento, devuelve la misma posicion.
size_t hash_proximo(const hash_t* hash, size_t pos_actual) {
    for (size_t i = pos_actual + 1; i < hash->capacidad; i++)
        if (hash->tabla[i].estado == OCUPADO)
            return i;
    return pos_actual;
}

// Si ya existe una clave en el hash, devuelve su posicion, y un bool "existia" true por la interfaz.
// De no existir previamente, devuelve la primer posicion disponible para agregar el elemento a la tabla, y el bool devuelto por la interfaz como false.
size_t hash_buscar_clave(const hash_t* hash, const char* clave, size_t pos, bool* existia) {
    size_t espacio_libre = 0;
    *existia = false;
    bool espacio_listo = false;
    for (size_t i = 0; i < hash->capacidad; i++) {
        if (!espacio_listo && hash->tabla[pos].estado == BORRADO) {
            espacio_libre = pos;
            espacio_listo = true;
        }
        else if (hash->tabla[pos].estado == OCUPADO && !strcmp(clave, hash->tabla[pos].clave)) {
            *existia = true;
            return pos;
        }
        else if (hash->tabla[pos].estado == VACIO) {
            if (espacio_listo) {
                return espacio_libre;
            }
            return pos;
        }
        pos++;
        if (pos == hash->capacidad)
            pos = 0;
    }
    return espacio_libre;
}
