#ifndef RAYH
#define RAYH
#include "vec3.h"

// Klasa ray (zrak) je srce našeg ray tracer-a.
// Sve što radimo svodi se na jedno pitanje: "Koje boje je ono što ovaj zrak pogodi?"
// Zrak matematički posmatramo kao funkciju p(t) = A + t*B.
class ray
{
    public:
        // Podrazumijevani konstruktor
        __device__ ray() {}
        
        // Konstruktor koji prima tačku polaska (A) i vektor pravca (B)
        __device__ ray(const vec3& a, const vec3& b) { A = a; B = b; }
        
        // Vraća tačku A — odakle zrak kreće (origin)
        __device__ vec3 origin() const       { return A; }
        
        // Vraća vektor B — u kom smjeru zrak ide (direction)
        __device__ vec3 direction() const    { return B; }
        
        // Najvažniji dio: vraća 3D tačku na zraku za bilo koji parametar 't'.
        // Ako je t = 0, nalazimo se na početku (u tački A).
        // Kako t raste u pozitivnom smjeru (t > 0), idemo naprijed kroz prostor duž vektora pravca B.
        // Ova linearna interpolacija nam omogućava da lako nađemo tačke presjeka sa sferama!
        __device__ vec3 point_at_parameter(float t) const { return A + t*B; }

        vec3 A; // Početak zraka (3D tačka)
        vec3 B; // Pravac zraka (3D vektor)
};

#endif
