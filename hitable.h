#ifndef HITABLEH
#define HITABLEH

// Pošto se 'hittable' i 'material' međusobno referenciraju (hitable sadrži pokazivač na materijal, 
// a materijal koristi hit_record podatke), ovdje se radi samo "forward declaration" klase material.
// Na taj način kompajler zna da ta klasa postoji, a izbjegava se beskonačna petlja pri uključivanju zaglavlja.
class material;

#include "ray.h"

// Ova struktura je ključna tačka spajanja geometrije i sjenčenja. 
// Kada zrak pogodi neki objekat u sceni, u 'hit_record' pakuju se svi detalji o tom sudaru.
// Ti podaci su kasnije neophodni da bi znali kako da odbijemo zrak i koju boju da proračunamo.
struct hit_record
{
    float t;             // t-parametar iz jednačine zraka P(t) = A + t*B. Predstavlja distancu do tačke udara.
    vec3 p;              // Tačna 3D tačka u prostoru gdje se desio sudar zraka i objekta.
    vec3 normal;         // Vektor normale u tački udara. Normala uvijek stoji pod uglom od 90 stepeni na površinu.
    material *mat_ptr;   // Pokazivač na materijal objekta koji je pogođen (da znamo da li je mat, metal ili staklo).
};

// Ovo je bazna apstraktna klasa (interfejs u C++ smislu) za sve objekte u sceni koje zrak može pogoditi.
// KLJUČNA CUDA IZMJENA: Dodajemo '__device__' oznaku ispred virtuelnih funkcija.
// To govori NVCC kompajleru da se ova funkcija i njene izvedene verzije moraju kompajlirati 
// isključivo za izvršavanje na GPU jezgrima, jer procesor (CPU) neće radisti provjeru sudara.
class hitable {
    public:
        // Čista virtuelna funkcija koju svaki geometrijski oblik (poput sfere) mora implementirati.
        // Funkcija vraća 'true' ako je zrak pogodio objekat unutar dozvoljenog opsega [t_min, t_max].
        // Opseg [t_min, t_max] je ključan da bismo ignorisali pogotke koji su iza kamere (t < 0) 
        // ili previše daleko (t > max_dist), kao i da riješimo problem "shadow acne" (sitnih grešaka u zaokruživanju).
        __device__ virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const = 0;
};

#endif

